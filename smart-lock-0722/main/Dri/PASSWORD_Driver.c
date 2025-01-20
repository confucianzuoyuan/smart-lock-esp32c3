#include "PASSWORD_Driver.h"

#include "stdint.h"
#include <inttypes.h>
#include <stdio.h>
#include <time.h>

int32_t hash_code(char *s) {
  int32_t h = 0;
  int i = 0;
  while (i < 19 + 8) {
    h = (h << 5) - h + ((uint8_t)s[i] | 0);
    i++;
  }
  return h;
}

uint8_t PASSWORD_ValidateTempPassword(int32_t temp) {
  // 获取当前时间
  time_t now;
  time(&now);
  printf("当前时间(以秒为单位)：%" PRIu64 "\n", now);
  // now是unix时间戳，以秒为单位
  uint64_t now_in_minutes = now / 60;
  printf("当前时间(以分钟为单位)：%" PRIu64 "\n", now_in_minutes);

  // 校验过去20分钟有没有符合条件的时间戳
  for (int k = 0; k < 20; k++) {
    // length of 2**64 - 1, +1 for nul.
    char buff[21];

    // 校验过去20分钟
    // copy to buffer
    sprintf(buff, "%" PRIu64, now_in_minutes - k);
    printf("buff: %s\r\n", buff);
    char *serial_number = "AtguiguSmartLock001";
    // 用于计算临时密码的key=serial_number+now_in_minutes
    char key[27] = {0};
    // 拷贝序列号
    for (int i = 0; i < 19; i++) {
      key[i] = serial_number[i];
    }
    // 拷贝now_in_minutes
    for (int j = 19; j < 27; j++) {
      key[j] = buff[j - 19];
    }
    /// 根据"序列号+时间戳"计算出一个i32的数值
    int32_t code = hash_code(key);
    /// 对100_0000求余，得到6位数字
    int32_t temp_password = ((code % 1000000) + 1000000) % 1000000;
    printf("%" PRId32 "\n", temp_password);

    if (temp_password == temp)
      return 0;
  }

  return 1;
}