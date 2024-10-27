# GetAudio
## 项目仅仅是一个捕获Windows系统音频输出并且生成文件的demo
### 详细说明
1. `WaveFile.h/cpp` 是把音频信息生成 `.wav` 文件的工具
2. 一共会捕获10秒

### 思路
1. 通过 Windows 的 COM 框架遍历音频设备并选择一个合适的并创建一个  `IMMDevice` (代码里是选了一个默认的)
2. 通过 `IMMDevice` 创建一个  `IAudioClient `接口,以便呈现、捕获和独占音频流
3. 这个demo的目的是捕获系统的输出音频流，所以需要用`IAudioClient `生成 `IAudioCaptureClient` 接口
4. 获取 音频流、音频流大小和音频流的格式
5. 生成文件/传输|压缩传输

### 相关资料文档
1. windows 的 捕获流 文档
2. windows 的 createFile 文档

### 肯定的后续
1. 将会支持简单的通过 udp 协议把音频源传输出去

### 可能的后续
- 因为仅仅是个demo,所以仅仅提供一些简单的实现用于参考
  1. 支持 AAC 等多种数据压缩
  2. 支持 TCP 等其他协议的数据传输
  3. 支持其他格式的文件保存功能(此条大概率不会)
