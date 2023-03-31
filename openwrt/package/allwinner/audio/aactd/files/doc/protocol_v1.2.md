上位机软件与设备端的通信协议。

# Version 1

对长度大于 1 byte 的内容，按小端序排列。

各字段与其长度为：

| [flag] | [version] | [command] | [type] | [data_len] | [data] | [checksum] |
| ------ | --------- | --------- | ------ | ---------- | ------ | ---------- |
| 1 byte | 1 byte    | 1 byte | 1 byte | 4 byte     | 不定   | 1 byte     |

- [flag]：固定为 0xAA。
- [version]：协议的版本号。当前版本为 1。
- [command]：区分对另一端进行读还是写操作。
- [type]：用来区分 [data] 段是什么内容。
- [data_len]：[data] 段的长度（单位为 byte）。当 [command] 为 `READ` 时，[data_len] 的值固定为 0。
- [data]：数据部分。长度由 [data_len] 指定。当 [command] 为 `READ` 时，[data] 段不存在。
- [checksum]：校验和。为前面所有内容相加后取低 8 位。

## [command]

| 名字      | 数值 | 说明                                        |
| --------- | ---- | ------------------------------------------- |
| CMD_READ  | 0    | 此时 [data_len] 固定为 0，[data] 段不存在。 |
| CMD_WRITE | 1    |                                             |

## [type]

可以为以下内容：

| 名字   | 数值 | 说明       |
| ------ | ---- | ---------- |
|        | 0    | （暂保留） |
| EQ_SW  | 1    | 软件 EQ    |
| DRC_HW | 2    | 硬件 DRC   |

### EQ_SW

[type] 为 `EQ_SW` 时，[data] 为软件 EQ 算法的参数，格式为：

| [global_enabled] | [filter_num] | [filter_args]                             |
| ---------------- | ------------ | ----------------------------------------- |
| 1 byte           | 1 byte       | [filter_num] * (单个 [filter_arg] 的长度) |

- [global_enabled]：是否全局使能 EQ 算法（0：不使能，1：使能）。
- [filter_num]：EQ 算法中的滤波器总数。不管某个滤波器当前是否使能，都需要算上。该字段不是为了表明当前使能了多少个滤波器，而是表明 EQ 算法中总共有多少滤波器可用，以防后续 EQ 算法的滤波器总数有增减。
- [filter_args]：滤波器的参数。共包含有 [filter_num] 个 [filter_arg]。

每一个 [filter_arg] 的格式为：

| [filter_type] | [frequency] | [gain] | [quality] | [enabled] |
| ------------- | ----------- | ------ | --------- | --------- |
| 1 byte        | 4 byte      | 4 byte | 4 byte    | 1 byte    |

- [filter_type]：滤波器类型（与 EQ 算法头文件和配置文件中的定义保持一致）。

  | 名字              | 数值 |
  | ----------------- | ---- |
  | LOWPASS_SHELVING  | 0    |
  | BANDPASS_PEAK     | 1    |
  | HIGHPASS_SHELVING | 2    |
  | LOWPASS           | 3    |
  | HIGHPASS          | 4    |

- [frequency]：频率。正整数，以 int32_t 保存。

- [gain]：增益。正/负整数，以 int32_t 保存。

- [quality]：品质因子，即 Q 值。原为浮点数，将原数值 *100 后以 int32_t 保存。

- [enabled]：表明该滤波器是否使能（0：不使能，1：使能）。

### DRC_HW

[type] 为 `DRC_HW` 时，[data] 为硬件 DRC 的寄存器参数，格式为：

| [reg_num] | [reg_args]                          |
| --------- | ----------------------------------- |
| 2 byte    | [reg_num] * (单个 [reg_arg] 的长度) |

- [reg_num]：寄存器数量。正整数，以 uint16_t 保存。
- [reg_args]：共包含 [reg_num] 个 [reg_arg]。

每一个 [reg_arg] 的格式为：

| [reg_offset] | [reg_value] |
| ------------ | ----------- |
| 4 byte       | 4 byte      |

- [reg_offset]：寄存器地址的偏移值（基于 audiocodec 寄存器的基地址）。以 uint32_t 保存。
- [reg_value]：寄存器数值。以 uint32_t 保存。