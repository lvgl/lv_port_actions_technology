#ifndef __SPINAND__H__
#define __SPINAND__H__

int spinand_storage_ioctl(const struct device *dev, uint8_t cmd, void *buff);

#endif /* __SPINAND__H__ */
