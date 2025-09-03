# 完整的暗场构建实现

## 概述

我已经为PHD2实现了一个完整的异步暗场库构建系统，替换了原来的占位符实现。这个系统提供了完整的JSON-RPC API，允许客户端程序化地构建相机传感器的暗场帧库。

## 实现的功能

### 1. 核心数据结构
- `DarkLibraryBuildOperation` 结构体：跟踪暗场库构建操作的状态
- 包含所有必要的参数、状态跟踪和进度信息
- 管理从暗场帧捕获到库创建的完整工作流程

### 2. 异步处理系统
- `DarkLibraryBuildThread` 类：后台处理线程
- 非阻塞操作，不干扰JSON-RPC接口
- 使用wxWidgets线程管理的正确线程处理

### 3. 完整的API方法
- `start_dark_library_build`：启动暗场库捕获序列
- `get_dark_library_status`：返回当前库状态和构建进度
- `cancel_dark_library_build`：取消正在运行的构建操作
- 增强现有方法：`load_dark_library`、`clear_dark_library`

### 4. 操作状态跟踪
- 使用wxMutex的线程安全操作跟踪
- 全面的状态状态：
  - `STARTING`：操作初始化
  - `CAPTURING_DARKS`：从相机捕获暗场帧
  - `BUILDING_MASTER_DARKS`：创建主暗场帧
  - `SAVING_LIBRARY`：保存暗场库到磁盘
  - `COMPLETED`：操作成功完成
  - `FAILED`：操作失败并显示错误
  - `CANCELLED`：操作被取消

### 5. 进度监控
- 实时进度跟踪，显示完成百分比
- 逐帧进度更新
- 当前曝光时间和帧数报告
- 每个阶段的详细状态消息

## 主要代码更改

### 在 `src/communication/network/event_server.cpp` 中：

1. **添加了 `DarkLibraryBuildOperation` 结构体**（第2512行）：
   - 跟踪操作参数和状态
   - 管理曝光持续时间列表
   - 线程安全的状态更新

2. **添加了 `DarkLibraryBuildThread` 类**（第2612行）：
   - 异步执行暗场库构建
   - 处理每个曝光时间的主暗场创建
   - 错误处理和取消支持

3. **完全重写了 `start_dark_library_build` 函数**（第2889行）：
   - 替换了TODO占位符
   - 启动异步构建过程
   - 返回操作ID用于跟踪

4. **增强了 `get_dark_library_status` 函数**（第2990行）：
   - 支持操作特定的状态查询
   - 详细的进度报告
   - 一般库状态信息

5. **添加了 `cancel_dark_library_build` 函数**（第3201行）：
   - 取消正在运行的构建操作
   - 线程安全的取消处理

6. **更新了函数映射表**（第5038行）：
   - 添加了新的取消函数到API

## 技术特性

### 线程安全
- 所有操作状态都由互斥锁保护
- 对相机和帧对象的线程安全访问
- 取消时的正确资源清理

### 错误处理
- 每个步骤的全面错误检查
- 相机捕获失败的优雅处理
- 错误或取消时的正确清理

### 内存管理
- 分配的暗场帧的自动清理
- 向相机对象的正确所有权转移
- 操作完成或失败时无内存泄漏

### 与现有代码的集成
- 使用来自 `DarksDialog` 的现有 `CreateMasterDarkFrame` 逻辑
- 与现有暗场库保存/加载基础设施集成
- 与现有相机暗场帧管理兼容

## 工作流程

1. **初始化**：创建操作结构体，确定曝光持续时间
2. **清理**：如果构建新库，清除现有暗场
3. **捕获循环**：对每个曝光时间：
   - 捕获指定数量的暗场帧
   - 平均像素值创建主暗场
   - 更新进度状态
4. **添加到相机**：将主暗场添加到相机的暗场集合
5. **保存库**：将暗场库保存到磁盘
6. **完成**：加载库并设置为使用状态

## 使用示例

```python
# 启动暗场库构建
result = send_request("start_dark_library_build", {
    "min_exposure": 1000,
    "max_exposure": 10000,
    "frame_count": 5,
    "notes": "会话暗场库"
})
operation_id = result["operation_id"]

# 监控进度
while True:
    status = send_request("get_dark_library_status", {
        "operation_id": operation_id
    })
    
    if status["status"] in ["completed", "failed", "cancelled"]:
        break
    
    print(f"进度: {status['progress']}% - {status['status_message']}")
    time.sleep(2)

# 加载完成的库
send_request("load_dark_library")
```

## 测试

包含的 `test_dark_library_api.py` 脚本提供了：
- API功能的全面测试
- 进度监控演示
- 错误处理验证
- 取消操作测试

## 总结

这个实现提供了一个完整的、生产就绪的暗场库构建系统，与PHD2现有基础设施无缝集成，同时为外部客户端提供现代的异步API。系统经过精心设计，具有适当的错误处理、线程安全和资源管理。
