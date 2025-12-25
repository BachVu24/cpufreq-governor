// ina226_i2c.c - INA226 driver for Linux Kernel 6.6+
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/device.h>

#define INA226_REG_SHUNT_VOLT   0x01
#define INA226_REG_BUS_VOLT     0x02
#define INA226_REG_POWER        0x03
#define INA226_REG_CURRENT      0x04

struct ina226_data {
        struct i2c_client *client;
        struct class *cls;
        struct device *dev;
};

static int ina226_read_reg(struct i2c_client *client, u8 reg)
{
        s32 ret = i2c_smbus_read_word_data(client, reg);
        if (ret < 0)
                return ret;

        return swab16(ret);  // byte swap
}

/* ───── sysfs attributes ───── */

static ssize_t shunt_current_show(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
        struct ina226_data *data = dev_get_drvdata(dev);
        int raw = ina226_read_reg(data->client, INA226_REG_CURRENT);
        if (raw < 0)
                return raw;

        return sprintf(buf, "%d\n", raw);
}

static ssize_t bus_voltage_show(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
        struct ina226_data *data = dev_get_drvdata(dev);
        int raw = ina226_read_reg(data->client, INA226_REG_BUS_VOLT);
        if (raw < 0)
                return raw;

        int mv = raw * 125 / 100;  // 1.25mV LSB
        return sprintf(buf, "%d\n", mv);
}

static ssize_t power_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
        struct ina226_data *data = dev_get_drvdata(dev);
        int raw = ina226_read_reg(data->client, INA226_REG_POWER);
        if (raw < 0)
                return raw;

        return sprintf(buf, "%d\n", raw);
}

static DEVICE_ATTR_RO(shunt_current);
static DEVICE_ATTR_RO(bus_voltage);
static DEVICE_ATTR_RO(power);

static struct attribute *ina226_attrs[] = {
        &dev_attr_shunt_current.attr,
        &dev_attr_bus_voltage.attr,
        &dev_attr_power.attr,
        NULL
};

static const struct attribute_group ina226_attr_group = {
        .attrs = ina226_attrs,
};

/* ───── driver probe/remove ───── */

static int ina226_probe(struct i2c_client *client)
{
        struct ina226_data *data;
        int ret;

        data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
        if (!data)
                return -ENOMEM;

        data->client = client;

        /* class_create in kernel 6.6 */
        data->cls = class_create("ina226");
        if (IS_ERR(data->cls))
                return PTR_ERR(data->cls);

        data->dev = device_create(data->cls, NULL, 0, data, "sensor0");
        if (IS_ERR(data->dev)) {
                class_destroy(data->cls);
                return PTR_ERR(data->dev);
        }

        ret = sysfs_create_group(&data->dev->kobj, &ina226_attr_group);
        if (ret) {
                device_unregister(data->dev);
                class_destroy(data->cls);
                return ret;
        }

        dev_info(&client->dev, "INA226 (kernel 6.6) driver loaded\n");
        return 0;
}

static void ina226_remove(struct i2c_client *client)
{
        struct ina226_data *data = i2c_get_clientdata(client);

        sysfs_remove_group(&data->dev->kobj, &ina226_attr_group);
        device_unregister(data->dev);
        class_destroy(data->cls);

        dev_info(&client->dev, "INA226 driver removed\n");
}

/* ───── Device Tree match ───── */

static const struct of_device_id ina226_of_match[] = {
        { .compatible = "mycorp,ina226_custom" },
        { }
};
MODULE_DEVICE_TABLE(of, ina226_of_match);

/* ───── I2C driver ───── */

static struct i2c_driver ina226_driver = {
        .driver = {
                .name = "ina226_custom",
                .of_match_table = ina226_of_match,
        },
        .probe = ina226_probe,
        .remove = ina226_remove,
};

module_i2c_driver(ina226_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("INA226 driver (Linux Kernel 6.6 compatible)");
