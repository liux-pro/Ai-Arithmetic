#define SET_BIT(reg, bit)    ((reg) |= (1 << (bit)))    // 设置指定的位
#define CLEAR_BIT(reg, bit)  ((reg) &= ~(1 << (bit)))   // 清除指定的位
#define TOGGLE_BIT(reg, bit) ((reg) ^= (1 << (bit)))    // 切换指定的位
#define CHECK_BIT(reg, bit)  (((reg) >> (bit)) & 1)     // 检查指定的位
#define IS_BIT_SET(reg, bit) ((reg) & (1 << (bit)))      // 检查位是否为1
#define IS_BIT_CLEAR(reg, bit) (!((reg) & (1 << (bit)))) // 检查位是否为0
