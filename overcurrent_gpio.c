// overcurrent_gpio.c - GPIO IRQ Overcurrent detector (Linux kernel 6.6+)

#include <linux/module.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

struct overcurrent_dev {
        struct device *dev;
        struct class *cls;
        struct device *sysfs_dev;

        struct gpio_desc *gpiod;
        int irq;
        int state;

        spinlock_t lock;
};

static ssize_t state_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
        struct overcurrent_dev *od = dev_get_drvdata(dev);
        unsigned long flags;
        int s;

        spin_lock_irqsave(&od->lock, flags);
        s = od->state;
        spin_unlock_irqrestore(&od->lock, flags);

        return sprintf(buf, "%d\n", s);
}

static DEVICE_ATTR_RO(state);

static struct attribute *oc_attrs[] = {
        &dev_attr_state.attr,
        NULL,
};

static const struct attribute_group oc_attr_group = {
        .attrs = oc_attrs,
};

static irqreturn_t oc_irq_handler(int irq, void *dev_id)
{
        struct overcurrent_dev *od = dev_id;
        unsigned long flags;

        spin_lock_irqsave(&od->lock, flags);
        od->state = gpiod_get_value(od->gpiod);
        spin_unlock_irqrestore(&od->lock, flags);

        dev_warn(od->dev, "overcurrent IRQ fired, state=%d\n", od->state);

        return IRQ_HANDLED;
}

static int overcurrent_probe(struct platform_device *pdev)
{
        int ret;
        struct overcurrent_dev *od;

        od = devm_kzalloc(&pdev->dev, sizeof(*od), GFP_KERNEL);
        if (!od)
                return -ENOMEM;

        od->dev = &pdev->dev;
        spin_lock_init(&od->lock);

        /* Read GPIO from Device Tree */
        od->gpiod = devm_gpiod_get(&pdev->dev, NULL, GPIOD_IN);
        if (IS_ERR(od->gpiod)) {
                dev_err(&pdev->dev, "Failed to get GPIO from DT\n");
                return PTR_ERR(od->gpiod);
        }

        od->irq = gpiod_to_irq(od->gpiod);
        if (od->irq < 0)
                return od->irq;

        dev_info(&pdev->dev, "Using IRQ %d\n", od->irq);

        /* Register IRQ */
        ret = devm_request_irq(&pdev->dev, od->irq,
                               oc_irq_handler,
                               IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                               "overcurrent_irq",
                               od);
        if (ret)
                return ret;

        /* Create class + sysfs */
        od->cls = class_create("overcurrent");
        if (IS_ERR(od->cls))
                return PTR_ERR(od->cls);

        od->sysfs_dev = device_create(od->cls, NULL, 0, od, "sensor0");
        if (IS_ERR(od->sysfs_dev)) {
                class_destroy(od->cls);
                return PTR_ERR(od->sysfs_dev);
        }

        ret = sysfs_create_group(&od->sysfs_dev->kobj, &oc_attr_group);
        if (ret) {
                device_unregister(od->sysfs_dev);
                class_destroy(od->cls);
                return ret;
        }

        dev_info(&pdev->dev, "overcurrent_gpio loaded successfully\n");
        platform_set_drvdata(pdev, od);

        return 0;
}

static int overcurrent_remove(struct platform_device *pdev)
{
	pr_info("overcurrent_gpio: remove called\n");
        struct overcurrent_dev *od = platform_get_drvdata(pdev);

        sysfs_remove_group(&od->sysfs_dev->kobj, &oc_attr_group);
        device_unregister(od->sysfs_dev);
        class_destroy(od->cls);

        dev_info(od->dev, "overcurrent_gpio removed\n");
	return 0;
}

static const struct of_device_id overcurrent_of_match[] = {
        { .compatible = "custom,overcurrent-gpio" },
        { }
};
MODULE_DEVICE_TABLE(of, overcurrent_of_match);

static struct platform_driver overcurrent_driver = {
        .driver = {
                .name = "overcurrent_gpio",
                .of_match_table = overcurrent_of_match,
        },
        .probe = overcurrent_probe,
        .remove = overcurrent_remove,
};

module_platform_driver(overcurrent_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("GPIO + IRQ Overcurrent Detector (Linux 6.6)");
