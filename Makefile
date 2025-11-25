obj-m := tree_governor.o

    # Đường dẫn tới thư mục build kernel
    KDIR := /lib/modules/$(shell uname -r)/build

    # Lệnh build chính
    all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

    # Lệnh clean
    clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
