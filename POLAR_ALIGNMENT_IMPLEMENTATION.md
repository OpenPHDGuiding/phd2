# 完善的极轴校准功能实现

## 概述

我已经完善了PHD2的极轴校准功能，将原来的占位符实现替换为完整的、集成现有UI工具的功能实现。这个系统提供了完整的JSON-RPC API，允许客户端程序化地执行三种不同类型的极轴校准。

## 实现的功能

### 1. 核心数据结构
- `PolarAlignmentOperation` 结构体：跟踪极轴校准操作的状态
- 支持三种校准类型：漂移校准、静态极轴校准、极轴漂移校准
- 包含完整的参数、状态跟踪和结果信息

### 2. 集成现有UI工具
- **漂移校准 (Drift Alignment)**：集成 `DriftTool` 和 `DriftToolWin`
- **静态极轴校准 (Static Polar Alignment)**：集成 `StaticPaTool` 和 `StaticPaToolWin`
- **极轴漂移校准 (Polar Drift Alignment)**：集成 `PolarDriftTool` 和 `PolarDriftToolWin`

### 3. 完整的API方法
- `start_drift_alignment`：启动漂移校准
- `start_static_polar_alignment`：启动静态极轴校准
- `start_polar_drift_alignment`：启动极轴漂移校准
- `get_polar_alignment_status`：获取校准状态和进度
- `cancel_polar_alignment`：取消正在进行的校准

### 4. 状态管理系统
- 线程安全的操作跟踪
- 实时状态更新和进度监控
- 与现有UI工具的状态同步

## 三种极轴校准方法

### 1. 漂移校准 (Drift Alignment)
**特点**：最准确的方法，通过分析赤道附近恒星的漂移来校准
**适用场景**：需要高精度校准的场合
**参数**：
- `direction`: 测量方向 ("east" 或 "west")
- `measurement_time`: 测量时间 (60-1800秒)

### 2. 静态极轴校准 (Static Polar Alignment)
**特点**：快速方法，通过测量RA轴相对于天极的偏移
**适用场景**：快速校准，适合便携式设备
**参数**：
- `hemisphere`: 半球 ("north" 或 "south")
- `auto_mode`: 自动模式 (true/false)

### 3. 极轴漂移校准 (Polar Drift Alignment)
**特点**：简单方法，通过分析天极附近恒星的漂移
**适用场景**：初学者友好，操作简单
**参数**：
- `hemisphere`: 半球 ("north" 或 "south")
- `measurement_time`: 测量时间 (60-1800秒)

## API详细说明

### `start_drift_alignment`
**目的**：启动漂移校准

**参数**：
- `direction` (string, 可选): 测量方向，默认 "east"
- `measurement_time` (int, 可选): 测量时间（秒），默认 300

**返回**：
```json
{
  "result": {
    "operation_id": 2001,
    "tool_type": "drift_alignment",
    "direction": "east",
    "measurement_time": 300,
    "status": "starting"
  }
}
```

### `start_static_polar_alignment`
**目的**：启动静态极轴校准

**参数**：
- `hemisphere` (string, 可选): 半球，默认 "north"
- `auto_mode` (bool, 可选): 自动模式，默认 true

**返回**：
```json
{
  "result": {
    "operation_id": 3001,
    "tool_type": "static_polar_alignment",
    "hemisphere": "north",
    "auto_mode": true,
    "status": "starting"
  }
}
```

### `start_polar_drift_alignment`
**目的**：启动极轴漂移校准

**参数**：
- `hemisphere` (string, 可选): 半球，默认 "north"
- `measurement_time` (int, 可选): 测量时间（秒），默认 300

**返回**：
```json
{
  "result": {
    "operation_id": 4001,
    "tool_type": "polar_drift_alignment",
    "hemisphere": "north",
    "measurement_time": 300,
    "status": "starting"
  }
}
```

### `get_polar_alignment_status`
**目的**：获取校准操作状态

**参数**：
- `operation_id` (int, 必需): 操作ID

**返回示例**（极轴漂移校准）：
```json
{
  "result": {
    "operation_id": 4001,
    "tool_type": "polar_drift_alignment",
    "status": "measuring",
    "progress": 65.5,
    "message": "Measuring drift - Error: 2.3 arcmin, Angle: 45.2 deg",
    "hemisphere": "north",
    "measurement_time": 300,
    "elapsed_time": 196.5,
    "polar_error_arcmin": 2.3,
    "adjustment_angle_deg": 45.2
  }
}
```

**状态值**：
- `starting`: 操作初始化
- `waiting_for_star`: 等待选择恒星
- `measuring`: 正在测量
- `adjusting`: 正在调整
- `completed`: 操作完成
- `failed`: 操作失败
- `cancelled`: 操作被取消

### `cancel_polar_alignment`
**目的**：取消校准操作

**参数**：
- `operation_id` (int, 必需): 要取消的操作ID

**返回**：
```json
{
  "result": {
    "success": true,
    "operation_id": 4001,
    "cancelled": true
  }
}
```

## 技术实现细节

### 1. UI工具集成
- 自动打开相应的工具窗口
- 与现有UI工具的状态同步
- 支持工具窗口的手动操作

### 2. 状态同步机制
- `GetDriftToolStatus()`: 获取漂移校准工具状态
- `GetPolarDriftToolStatus()`: 获取极轴漂移工具状态
- `GetStaticPaToolStatus()`: 获取静态极轴校准工具状态

### 3. 进度计算
- **漂移校准**: 基于测量时间计算进度
- **极轴漂移校准**: 基于测量时间和实时结果
- **静态极轴校准**: 基于捕获的位置数量

### 4. 结果提取
- 从工具窗口实时提取测量结果
- 极轴误差（角分）和调整角度（度）
- 测量进度和状态消息

### 5. 错误处理
- 工具窗口创建失败的处理
- 设备连接检查
- 参数验证

### 6. 资源管理
- 操作完成后的清理
- 工具窗口的生命周期管理
- 内存泄漏防护

## 使用示例

### 基本极轴漂移校准
```python
# 启动极轴漂移校准
result = send_request("start_polar_drift_alignment", {
    "hemisphere": "north",
    "measurement_time": 300
})
operation_id = result["operation_id"]

# 监控进度
while True:
    status = send_request("get_polar_alignment_status", {
        "operation_id": operation_id
    })
    
    print(f"状态: {status['status']}")
    print(f"进度: {status['progress']}%")
    
    if 'polar_error_arcmin' in status:
        print(f"极轴误差: {status['polar_error_arcmin']:.1f} 角分")
        print(f"调整角度: {status['adjustment_angle_deg']:.1f} 度")
    
    if status['status'] in ['completed', 'failed', 'cancelled']:
        break
    
    time.sleep(5)
```

### 静态极轴校准
```python
# 启动静态极轴校准
result = send_request("start_static_polar_alignment", {
    "hemisphere": "north",
    "auto_mode": true
})

# 监控进度...
```

### 取消操作
```python
# 取消正在进行的校准
cancel_result = send_request("cancel_polar_alignment", {
    "operation_id": operation_id
})
print(f"取消成功: {cancel_result['cancelled']}")
```

## 总结

这个实现提供了：

1. **完整的功能覆盖** - 支持PHD2的所有三种极轴校准方法
2. **无缝UI集成** - 与现有工具窗口完美集成
3. **实时状态监控** - 提供详细的进度和结果信息
4. **线程安全设计** - 支持并发操作和状态查询
5. **完整的错误处理** - 优雅处理各种异常情况
6. **向后兼容性** - 不影响现有的UI操作方式

这使得PHD2的极轴校准功能可以通过API完全自动化，为天文摄影爱好者提供了更灵活的校准选择。
