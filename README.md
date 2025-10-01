## `aarc-rc-converter`: AARC 至轨交棋地图转换工具

(Momochai, 2025.9.22)

### 编译

直接用 CMake 编译即可。 

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
|`max_length`|交路的最大长度 (车站与参考点的总数；下同) 参数 (一般情况下该参数无需调整；如果生成的交路不完整，请调大这一数值)。|`128`|
|`segmentation_length`|对复杂交路分段处理时的交路最大长度（该数值应大于轨交棋中随机数的最大值）。|`32`|
|`group_distance`|将多个车站合并的距离阈值 (单位：`30px`).|`1.0`|
|`merge_consecutive_duplicates`|如果同一交路的相邻两站是同一车站且该设置项为 `true`，合并之。|`true`|
|`link_modes`|对不同车站连线的处理；具体见样例。支持五种不同的连线：粗线 `ThickLine`，细线 `ThinLine`，虚线 (原色) `DottedLine1`，虚线 (覆盖) `DottedLine2`，车站团 `Group`；对每种连线有三种处理方式：合并 `Group`，(用一条线路) 连接 `Connect`，忽略 `None`.| 粗线：连接，细线：连接，虚线：忽略，车站团：合并
|`friend_lines`|可跨线运行的线路列表。(可以用输入文件中的线路编号或线路名称表示线路；下同。)|无|
|`merged_lines`|贯通运行的线路列表。|无|
|`segmented_lines`|分段处理的线路列表。|无|

#### 配置文件例

```JSON
{
  "max_length": 200,
  "segmentation_length": 40,
  "group_distance": 2.0,
  "merge_consecutive_duplicates": true,
  "link_modes": {
    "ThickLine": "Group",
    "ThinLine": "Connect",
    "DottedLine1": "Connect",
    "DottedLine2": "None",
    "Group": "Group"
  },
  "friend_lines": [
    ["1 号线", "2 号线"],
    [147, 65]
  ],
  "merged_lines": [
    ["机场快线", 257]
  ],
  "segmented_lines": [
    53,
    "轻轨市区线"
  ]
}
```

#### 小技巧

- 在 AARC 中给所有线路取名，这样在配置文件中可以直接用线路名称表示线路，使用更方便。
- 如果有一端呈“灯泡线”状的线路 (或者其他自己是自己的支线的情况)，可以在 `friend_lines` 中设置其与自身跨线运行来达到预期效果；或者，将灯泡线的一支独立成一条支线，利用转换工具对支线自动跨线的处理实现功能。
- 将 `group_distance` 设为稍大于线路宽度的数值较为稳妥。
- `merged_lines` 和 `friend_lines` 的实质区别在于，`friend_lines` 跨线需要跨线前后方向偏转角为锐角，而 `merged_lines` 不需要。
- `merge_consecutive_duplicates` 选项主要是为了处理在 AARC 中画图时同一车站团中的不同车站点均加入了某一条线路的情况。如果确实存在在同一站连续停两次的交路，可以将它设为 `false`，但需要额外留意上述情况。
- 分支或跨线情况较多的线路如果交路种类过多 (如果是带多个支线的环线，可行的交路甚至可能有无穷多种)，可以将涉及到的线路加入 `segmented_lines` 列表。转换工具会用相互部分重叠的短交路覆盖整条线路，以对有限的随机数范围达到等价的效果。