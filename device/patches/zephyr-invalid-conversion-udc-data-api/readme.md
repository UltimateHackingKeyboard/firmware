Resolves the following warnings:

```
nrfconnect/zephyr/include/zephyr/drivers/usb/udc.h:403:38: warning: invalid conversion from 'void*' to 'udc_data*' [-fpermissive]
  403 |         struct udc_data *data = dev->data;
      |                                 ~~~~~^~~~
      |                                      |
      |                                      void*
```

Original fix:
https://github.com/zephyrproject-rtos/zephyr/pull/69490/files
