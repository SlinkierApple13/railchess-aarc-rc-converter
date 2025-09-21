## `aarc-rc-converter`: AARC 至轨交棋地图转换工具

(Momochai, 2025.9.21)

### 编译

直接用 `cmake` 编译即可。 

### 使用说明

该工具有两种使用方式。

一种是直接在命令行参数中指定输入文件、输出文件与配置文件：
```
./aarc-rc-converter <输入文件> <输出文件> [--config <配置文件>]
```
另一种是直接打开工具后再指定输入文件、输出文件与配置文件。
```
./aarc-rc-converter
```
其中，输入文件为 AARC 存档的 JSON 工程文件，输出文件指定了轨交棋地图文件的输出位置；配置文件为可选项，包含对转换行为的自定义设置。

### 配置文件

配置文件是一个 JSON 文件，包含转换期间可能用到的自定义设置。如果一些设置项没有在配置文件中包含，或者不指定配置文件，转换工具会采用默认设置。

|设置项|用途|默认|
|---|---|---|
|`maximum_service_length`|一条交路的最大长度；超过该数值的交路会被截断。|`256`|
|`group_distance`|将多个车站合并的距离阈值 (单位：`30px`).|`1.0`|
|`merge_consecutive_duplicates`|是否允许同一交路的相邻两站是同一个车站。|`true`|
|`link_modes`|对不同车站连线的处理；具体见样例。支持五种不同的连线：粗线 `ThickLine`，细线 `ThinLine`，虚线 (原色) `DottedLine1`，虚线 (覆盖) `DottedLine2`，车站团 `Group`；对每种连线有三种处理方式：合并 `Group`，(用一条线路) 连接 `Link`，忽略 `None`.| 粗线：连接，细线：连接，虚线：忽略，车站团：合并
|`friend_lines`|可跨线运行的线路列表。可以用输入文件中的线路编号或线路名称 (若有) 表示线路。|无|
|`merged_lines`|贯通运行的线路列表。可以用输入文件中的线路编号或线路名称 (若有) 表示线路。|无|

#### 配置文件例

```JSON
{
  "maximum_service_length": 64,
  "group_distance": 2.0,
  "merge_consecutive_duplicates": true,
  "link_modes": {
    "ThickLine": "Group",
    "ThinLine": "Link",
    "DottedLine1": "None",
    "DottedLine2": "None",
    "Group": "Group"
  },
  "friend_lines": [
    ["1", "2"],     // 用线路名表示
    [147, 65]       // 用线路编号表示
  ],
  "merged_lines": [
    ["9", 257]      // 混合表示
  ]
}
```
