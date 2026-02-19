import meep as mp
import numpy as np
import matplotlib.pyplot as plt

# ==========================================
# 模块 1: JONSWAP 海谱生成器 (物理核心)
# ==========================================
def generate_jonswap_surface(length_m, dx_m, wind_speed, fetch_km):
    """
    基于 JONSWAP 谱生成一维随机海面高度
    :param length_m: 海面总长度 (米)
    :param dx_m: 离散采样间隔 (米)
    :param wind_speed: 19.5m 处风速 (m/s)
    :param fetch_km: 风区长度 (km) - JONSWAP 特有参数
    :return: x (坐标数组), h (高度数组)
    """
    g = 9.81              # 重力加速度
    fetch_m = fetch_km * 1000.0
    
    # 1. 计算无量纲风区长度 (Dimensionless Fetch)
    X_tilde = (g * fetch_m) / (wind_speed**2)
    
    # 2. 计算峰值频率 (Peak Angular Frequency, wp)
    # 经验公式: wp = 22 * (g/U) * (X_tilde)^-0.33
    wp = 22 * (g / wind_speed) * (X_tilde**(-0.33))
    
    # 3. 计算 Phillips 常数 alpha
    # 经验公式: alpha = 0.076 * (X_tilde)^-0.22
    alpha = 0.076 * (X_tilde**(-0.22))
    
    # 4. 构建频率轴
    # 波数范围：从基频到奈奎斯特频率
    k_min = 2 * np.pi / length_m
    k_max = 2 * np.pi / (2 * dx_m)
    dk = 2 * np.pi / length_m
    k_arr = np.arange(k_min, k_max, dk)
    
    # 深水波色散关系: w^2 = g*k
    w_arr = np.sqrt(g * k_arr)
    
    # 5. 计算 JONSWAP 谱密度 S(w)
    # (a) PM 基础谱
    S_pm = (alpha * g**2 / (w_arr**5)) * np.exp(-1.25 * (wp / w_arr)**4)
    
    # (b) 峰值增强因子 (Peak Enhancement)
    gamma = 3.3  # 标准 JONSWAP 参数
    sigma = np.where(w_arr <= wp, 0.07, 0.09)
    r = np.exp(-(w_arr - wp)**2 / (2 * sigma**2 * wp**2))
    enhancement = gamma ** r
    
    S_jonswap = S_pm * enhancement
    
    # 6. 随机相位合成海面 (Spectral Synthesis)
    # 幅值 A = sqrt(2 * S(w) * dw)
    # dw 近似为 group_velocity * dk，但在深水波中简单处理 w 的微分
    # 这里使用离散求和法合成
    dw = np.diff(w_arr, prepend=w_arr[0])
    amplitudes = np.sqrt(2 * S_jonswap * dw)
    phases = np.random.uniform(0, 2*np.pi, size=len(k_arr))
    
    x = np.arange(0, length_m, dx_m)
    h = np.zeros_like(x)
    
    # 叠加所有频率分量
    # 注意：对于长距离，建议使用 IFFT 加速，但几百米范围内循环求和更直观
    print(f"正在合成海面 (JONSWAP): 风速={wind_speed}m/s, 风区={fetch_km}km...")
    for i in range(len(k_arr)):
        h += amplitudes[i] * np.cos(k_arr[i] * x + phases[i])
        
    return x, h

# ==========================================
# 模块 2: FDTD 仿真配置 (Meep 核心)
# ==========================================
def run_fdtd_simulation():
    # --- A. 仿真参数设置 ---
    resolution = 20        # 分辨率 (像素/米)，建议 >= lambda/20
    freq_ghz = 0.3         # 频率 0.3 GHz (300 MHz) -> lambda = 1m
    
    # 物理空间尺寸
    L_x = 200.0            # 仿真区域长度 (米) - 缩短以便演示
    L_z = 30.0             # 仿真区域高度 (米)
    dpml = 2.0             # PML 吸收层厚度 (米)
    
    # 海况参数
    wind_speed = 15.0      # 风速 (m/s)
    fetch_km = 50.0        # 风区 (km)
    
    # --- B. 生成几何结构 ---
    # 生成海面高度数据
    dx = 1.0 / resolution
    x_surf, h_surf = generate_jonswap_surface(L_x + 2*dpml, dx, wind_speed, fetch_km)
    
    # 在 Meep 中构建海面实体 (Prism)
    # Meep 坐标系中心是 (0,0)，我们需要把数据平移到坐标系中
    # 构造多边形顶点：从左下 -> 右下 -> 右上(海面) -> 左上(海面)
    
    # 调整 x 坐标以匹配 Meep 的 cell 定义 (从 -Lx/2 到 Lx/2)
    x_meep = x_surf - (L_x/2 + dpml)
    
    # 顶点列表
    vertices = [mp.Vector3(-L_x/2 - dpml, -L_z/2 - dpml)] # 左下底
    vertices.append(mp.Vector3(L_x/2 + dpml, -L_z/2 - dpml)) # 右下底
    
    # 添加海面上的点 (从右向左添加，闭合多边形)
    # 注意：h_surf 通常在 0 附近波动，我们需要将其放在仿真区域的下半部分
    base_water_level = -5.0 # 平均海平面设在 z = -5m
    
    # 为了保证多边形闭合顺序，我们反向遍历海面点
    for i in range(len(x_meep)-1, -1, -1):
        # 限制海浪高度，防止溢出网格
        z_val = base_water_level + h_surf[i]
        if z_val > L_z/2 - dpml: z_val = L_z/2 - dpml - 0.5 # 简单的削顶保护
        vertices.append(mp.Vector3(x_meep[i], z_val))
        
    # 定义材质：PEC (完美电导体)
    sea_geometry = [mp.Prism(vertices, height=mp.inf, material=mp.metal)]
    
    # --- C. 设置源与边界 ---
    # 仿真区域大小
    cell_size = mp.Vector3(L_x + 2*dpml, L_z + 2*dpml)
    
    # 边界条件：全向 PML
    pml_layers = [mp.PML(dpml)]
    
    # 信号源：连续波 (CW) 逐渐开启
    # 放置在左侧，高度为 5m (相对于平均海平面 z=-5, 所以绝对坐标 z=0)
    source_freq = freq_ghz # Meep 单位中，c=1，如果长度单位是m，时间单位就是m/c
    # 这里我们简化单位制：1 unit = 1 meter. c = 1.
    # 频率 f = 0.3 GHz -> lambda = 1m. 在 Meep 中 f = 1/lambda = 1.
    fcen = 1.0  # 对应 300MHz (波长1m)
    
    sources = [mp.Source(
        mp.ContinuousSource(frequency=fcen, width=10), # width是开启时的斜坡宽度
        component=mp.Ez, # 水平极化 (对于2D TMz)
        center=mp.Vector3(-L_x/2 + 10, 0), # 源位置: 左侧向内10m，z=0 (海面以上5m)
        size=mp.Vector3(0, 0) # 点源
    )]
    
    # --- D. 初始化仿真 ---
    sim = mp.Simulation(
        cell_size=cell_size,
        boundary_layers=pml_layers,
        geometry=sea_geometry,
        sources=sources,
        resolution=resolution
    )
    
    # --- E. 运行仿真 ---
    print("开始 FDTD 仿真 (Meep)...")
    # 运行直到场传遍整个区域 (大约 L_x + 时间余量)
    sim.run(until=L_x + 50)
    
    # --- F. 数据提取与可视化 ---
    print("仿真完成，正在绘图...")
    
    # 获取电场数据 (实部)
    ez_data = sim.get_array(center=mp.Vector3(), size=cell_size, component=mp.Ez)
    
    # 获取介电常数数据 (用于画海面轮廓)
    eps_data = sim.get_array(center=mp.Vector3(), size=cell_size, component=mp.Dielectric)
    
    plt.figure(figsize=(12, 6))
    
    # 绘制电场 (伪彩图)
    # 使用 RdBu 颜色映射，中心为白色表示0
    plt.imshow(np.real(ez_data).T, interpolation='spline36', cmap='RdBu', 
               extent=[-L_x/2 - dpml, L_x/2 + dpml, -L_z/2 - dpml, L_z/2 + dpml],
               origin='lower', vmin=-0.1, vmax=0.1)
    
    # 叠加海面轮廓 (通过介电常数掩膜)
    # Meep中 metal 的 epsilon 是无穷大，get_array 会返回很大的数
    plt.contour(np.real(eps_data).T, levels=[10], colors='black', linewidths=2,
                extent=[-L_x/2 - dpml, L_x/2 + dpml, -L_z/2 - dpml, L_z/2 + dpml],
                origin='lower')
    
    plt.colorbar(label='Electric Field (Ez)')
    plt.title(f'FDTD Simulation over JONSWAP Sea Surface\nWind={wind_speed}m/s, Freq=300MHz')
    plt.xlabel('Distance x (m)')
    plt.ylabel('Height z (m)')
    
    # 标注源位置
    plt.plot(-L_x/2 + 10, 0, 'go', label='Transmitter')
    plt.legend()
    
    plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    run_fdtd_simulation()