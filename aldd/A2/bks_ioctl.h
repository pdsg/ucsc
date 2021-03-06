#if !defined BKS_IOCTL_H
#define BKS_IOCTL_H

#include <linux/ioctl.h>

#define BKS_DRIVER_MAGIC (218)

#define BKS_IOCTL_GET_BLINK_FREQUENCY _IOR(BKS_DRIVER_MAGIC, 1, int*)
#define BKS_IOCTL_SET_BLINK_FREQUENCY _IOW(BKS_DRIVER_MAGIC, 2, int*)

#define BKS_IOCTL_GET_BLINK_PATTERN _IOR(BKS_DRIVER_MAGIC, 3, int*)
#define BKS_IOCTL_SET_BLINK_PATTERN _IOW(BKS_DRIVER_MAGIC, 4, int*)

#define BKS_IOCTL_GET_BLINK_STATE _IOR(BKS_DRIVER_MAGIC, 5, int*)

#endif // !defined BKS_IOCTL_H

