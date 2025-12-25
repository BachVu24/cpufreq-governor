/*
 * Tree Governor (fixed & improved with per-core tree logging)
 * - Đọc kcpustat đúng cách (kcpustat_cpu)
 * - Linear/adaptive scaling target frequency
 * - Polling dynamic (fast/slow) dựa trên last_max_load
 * - Print per-core load as tree with current freq & polling mode
 */

#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/kernel_stat.h>
#include <linux/workqueue.h>
#include <linux/ktime.h>
#include <asm/div64.h>

MODULE_AUTHOR("Your Name Here");
MODULE_DESCRIPTION("Tree Governor with per-core tree logging and dynamic frequency scaling");
MODULE_LICENSE("GPL");

/* Runtime-tunable parameters */
unsigned int fast_poll_period_ms = 30;  
unsigned int slow_poll_period_ms = 100; 
unsigned int idle_poll_threshold = 12;  
unsigned int log_suppression_ms = 1000;  

module_param(fast_poll_period_ms, uint, 0644);
MODULE_PARM_DESC(fast_poll_period_ms, "FAST Polling period in ms.");
module_param(slow_poll_period_ms, uint, 0644);
MODULE_PARM_DESC(slow_poll_period_ms, "SLOW Polling period in ms.");
module_param(idle_poll_threshold, uint, 0644);
MODULE_PARM_DESC(idle_poll_threshold, "Max load % for slow polling.");
module_param(log_suppression_ms, uint, 0644);
MODULE_PARM_DESC(log_suppression_ms, "Min ms between verbose logs.");

/* Internal scaling constants */
#define UTIL_SCALE 100  

/* Per-policy data */
struct tree_gov_data {
	struct cpufreq_policy *policy;
	unsigned int current_target_freq;
	struct delayed_work update_work;
	u64 *last_cpu_active_time;  
	u64 *last_cpu_total_time;   
	unsigned int last_max_load; 
	ktime_t last_log_time;      
	spinlock_t lock;            
};

/* Forward */
static void tree_gov_update_func(struct work_struct *work);

/* Read per-cpu times */
static void read_cpu_times(unsigned int cpu, u64 *active, u64 *total)
{
	u64 user, nice, system, idle, iowait, irq, softirq;

	user    = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	nice    = kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];
	system  = kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	idle    = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	iowait  = kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
	irq     = kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	softirq = kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];

	*active = user + nice + system + irq + softirq + iowait;
	*total  = *active + idle;
}

/* Compute per-cpu utilization (0..100%) */
static unsigned int get_cpu_utilization(unsigned int cpu, struct tree_gov_data *data)
{
	u64 cur_active = 0, cur_total = 0;
	u64 active_delta = 0, total_delta = 0;
	unsigned int util = 0;

	read_cpu_times(cpu, &cur_active, &cur_total);
	active_delta = cur_active - data->last_cpu_active_time[cpu];
	total_delta  = cur_total  - data->last_cpu_total_time[cpu];

	data->last_cpu_active_time[cpu] = cur_active;
	data->last_cpu_total_time[cpu]  = cur_total;

	if (total_delta > 0) {
		util = (unsigned int)div64_u64(active_delta * UTIL_SCALE, total_delta);
		if (util > 100) util = 100;
	} else {
		util = 0;
	}

	return util;
}

/* Decide target frequency based on max per-core load, linear scaling */
static unsigned int decide_target_freq(struct cpufreq_policy *policy, struct tree_gov_data *data)
{
	unsigned int cpu, max_load = 0;
	unsigned int per_cpu_util;
	unsigned int target_freq;
	unsigned long min = policy->min;
	unsigned long max = policy->max;

	for_each_cpu(cpu, policy->cpus) {
		per_cpu_util = get_cpu_utilization(cpu, data);
		if (per_cpu_util > max_load)
			max_load = per_cpu_util;
	}

	if (max_load > 100) max_load = 100;

	spin_lock(&data->lock);
	data->last_max_load = max_load;
	spin_unlock(&data->lock);

	if (max <= min) {
		target_freq = (unsigned int)min;
	} else {
		u64 diff = (u64)(max - min);
		u64 scaled = div64_u64(diff * (u64)max_load, 100);
		u64 freq64 = (u64)min + scaled;
		if (freq64 > UINT_MAX) target_freq = (unsigned int)max;
		else target_freq = (unsigned int)freq64;
	}

	return target_freq;
}

/* Print per-core tree + current target freq + polling mode */
static void tree_gov_print_status(struct tree_gov_data *data, struct cpufreq_policy *policy)
{
	int cpu;
	unsigned int freq;
	char *mode;

	spin_lock(&data->lock);
	freq = data->current_target_freq;
	if (data->last_max_load <= idle_poll_threshold)
		mode = "SLOW";
	else
		mode = "FAST";
	spin_unlock(&data->lock);

	pr_info("tree_gov: CPU%u status: target freq = %u Hz, polling = %s, max load = %u%%\n",
	        policy->cpu, freq, mode, data->last_max_load);

	pr_info("tree_gov: per-core load:\n");
	for_each_cpu(cpu, policy->cpus) {
		unsigned int util = get_cpu_utilization(cpu, data);
		char *prefix = (cpu == cpumask_last(policy->cpus)) ? "└─" : "├─";
		pr_info(" %s CPU%u: %u%%\n", prefix, cpu, util);
	}
}

/* Worker function */
static void tree_gov_update_func(struct work_struct *work)
{
	struct tree_gov_data *data = container_of(work, struct tree_gov_data, update_work.work);
	struct cpufreq_policy *policy = data->policy;
	unsigned int new_freq;
	unsigned long fast_j = msecs_to_jiffies(max(1u, fast_poll_period_ms));
	unsigned long slow_j = msecs_to_jiffies(max(1u, slow_poll_period_ms));
	unsigned long next_delay_j;
	ktime_t now = ktime_get();

	new_freq = decide_target_freq(policy, data);

	spin_lock(&data->lock);
	if (new_freq != data->current_target_freq) {
		unsigned int old = data->current_target_freq;
		spin_unlock(&data->lock);

		cpufreq_driver_target(policy, new_freq, CPUFREQ_RELATION_L);

		spin_lock(&data->lock);
		data->current_target_freq = new_freq;
		spin_unlock(&data->lock);

		if (ktime_to_ms(ktime_sub(now, data->last_log_time)) >= log_suppression_ms) {
			pr_info("tree_gov: CPU%u freq %u -> %u Hz\n", policy->cpu, old, new_freq);
			data->last_log_time = now;
		}
	} else {
		spin_unlock(&data->lock);
	}

	/* Print per-core tree, rate-limited */
	if (ktime_to_ms(ktime_sub(now, data->last_log_time)) >= log_suppression_ms) {
		tree_gov_print_status(data, policy);
		data->last_log_time = now;
	}

	spin_lock(&data->lock);
	if (data->last_max_load <= idle_poll_threshold)
		next_delay_j = slow_j;
	else
		next_delay_j = fast_j;
	spin_unlock(&data->lock);

	schedule_delayed_work(&data->update_work, next_delay_j);
}

/* init per-policy */
static int tree_gov_init(struct cpufreq_policy *policy)
{
	struct tree_gov_data *data;
	unsigned int cpu;
	int nr = nr_cpu_ids;

	if (policy->governor_data)
		return -EEXIST;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) return -ENOMEM;

	data->policy = policy;
	spin_lock_init(&data->lock);
	data->last_max_load = 0;
	data->last_log_time = ktime_get();

	data->last_cpu_active_time = kcalloc(nr, sizeof(u64), GFP_KERNEL);
	data->last_cpu_total_time  = kcalloc(nr, sizeof(u64), GFP_KERNEL);
	if (!data->last_cpu_active_time || !data->last_cpu_total_time) {
		kfree(data->last_cpu_active_time);
		kfree(data->last_cpu_total_time);
		kfree(data);
		return -ENOMEM;
	}

	for_each_cpu(cpu, policy->cpus) {
		u64 act = 0, tot = 0;
		read_cpu_times(cpu, &act, &tot);
		data->last_cpu_active_time[cpu] = act;
		data->last_cpu_total_time[cpu]  = tot;
	}

	if (policy->cur >= policy->min && policy->cur <= policy->max)
		data->current_target_freq = policy->cur;
	else
		data->current_target_freq = policy->min;

	policy->governor_data = data;

	INIT_DELAYED_WORK(&data->update_work, tree_gov_update_func);
	schedule_delayed_work(&data->update_work, msecs_to_jiffies(max(1u, fast_poll_period_ms)));

	pr_info("tree_gov: CPU%u initialized. CPUs: %*pbl. initial target %u Hz\n",
	        policy->cpu, cpumask_pr_args(policy->cpus), data->current_target_freq);

	return 0;
}

/* exit per-policy */
static void tree_gov_exit(struct cpufreq_policy *policy)
{
	struct tree_gov_data *data = policy->governor_data;

	if (!data)
		return;

	cancel_delayed_work_sync(&data->update_work);

	kfree(data->last_cpu_active_time);
	kfree(data->last_cpu_total_time);
	kfree(data);
	policy->governor_data = NULL;

	pr_info("tree_gov: CPU%u exited\n", policy->cpu);
}

/* governor struct */
static struct cpufreq_governor tree_governor = {
	.name = "tree",
	.init = tree_gov_init,
	.exit = tree_gov_exit,
	.owner = THIS_MODULE,
};

/* module init/exit */
static int __init tree_gov_module_init(void)
{
	int ret = cpufreq_register_governor(&tree_governor);
	if (ret)
		pr_err("tree_gov: failed to register governor: %d\n", ret);
	else
		pr_info("tree_gov: registered\n");
	return ret;
}

static void __exit tree_gov_module_exit(void)
{
	cpufreq_unregister_governor(&tree_governor);
	pr_info("tree_gov: unregistered\n");
}

module_init(tree_gov_module_init);
module_exit(tree_gov_module_exit);
