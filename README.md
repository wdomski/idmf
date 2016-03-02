*Author: Wojciech Domski*

*Markdown flavoured text. Use any Markdown editor to get preview.*

# Driver
## Prerequisites

You need to have Ubuntu with XOR architecture installed.
Absolute minimum is Xenomai

## Compilation

To compile the driver run in the directory

```
make all
```

After this opperation a set of files will be createt and 
among them you will find: *idmf_drv.ko* and *app*.

## Running the driver

To runn the driver you should invoke 

```
sudo insmod ./idmf_drv.ko
```

This will result with loading the driver.
**Please keep in mind that the Xenomai kernel 
should be running**

To test the driver with basic functionality you should run
test application *app* that was created during compilation.

```
sudo ./app
```

It will run the application. To test a specific device just 
pass it as a parameter

```
sudo ./app idmf1
```

It will open *idmf1* device.

The first device is *idmf0*

## Removing the driver

To remove the driver:

```
sudo rmmod idmf_drv
```

## Diagnostics

When diagnosing the driver always consult the Linux syslog

```
tail /var/log/syslog
```

# API

API was created to use the driver in user-space.

**Please keep in mind that for the ADC you should use 
delays which does not block the kernel. 
for more information go to the 
idmf_api.h, idmf_api.c and app.c files**
