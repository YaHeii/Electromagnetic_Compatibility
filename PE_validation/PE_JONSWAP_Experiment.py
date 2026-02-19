import numpy as np
import scipy.fft as fft
import matplotlib.pyplot as plt
import pandas as pd

class PLST_PE_Solver:
    def __init__(self, freq_ghz, dx, dz, max_z, max_range):
        """
        初始化 PLST-PE 求解器
        """
        self.c = 299792458.0
        self.freq = freq_ghz * 1e9
        self.k0 = 2 * np.pi * self.freq / self.c
        self.dx = dx
        self.dz = dz
        self.max_z = max_z
        self.max_range = max_range
        
        # 网格设置 (物理空间 z > 0，共 nz 个点)
        self.nz = int(max_z / dz)
        self.fft_size = 2 * self.nz  # 为镜像法预留一倍空间
        
        # 物理高度轴 (仅正半轴)
        self.z = np.arange(self.nz) * self.dz
        
        # Wavenumber (kz) 轴，使用 scipy.fft.fftfreq 自动处理交错排序
        self.kz = fft.fftfreq(self.fft_size, d=self.dz) * 2 * np.pi
        
        # 初始化场向量
        self.u = np.zeros(self.fft_size, dtype=np.complex128)
        
        self._setup_absorber()

    def _setup_absorber(self):
        """设置顶部吸收层 (Hanning Window)"""
        self.absorber = np.ones(self.nz)
        absorb_layer_thickness = int(self.nz * 0.25) # 顶部 25% 吸收
        start_idx = self.nz - absorb_layer_thickness
        
        window = 0.5 * (1 + np.cos(np.pi * np.arange(absorb_layer_thickness) / absorb_layer_thickness))
        self.absorber[start_idx:] = window

    def calc_fresnel(self, grazing_angle_rad, eps_sea=80.0 - 1j*4.0):
        """计算菲涅尔反射系数 (水平极化 TE)"""
        sin_t = np.sin(grazing_angle_rad)
        cos_t = np.cos(grazing_angle_rad)
        num = sin_t - np.sqrt(eps_sea - cos_t**2 + 0j)
        den = sin_t + np.sqrt(eps_sea - cos_t**2 + 0j)
        return num / den

    def init_gaussian_beam(self, antenna_z_phys, h_surf_0, beam_width_deg=2.0, tilt_deg=0.0):
        """
        初始化高斯波束
        :param antenna_z_phys: 天线绝对物理高度
        :param h_surf_0: 发射点(x=0)的海面绝对高度
        """
        # 转换到网格相对高度 (zeta = z_phys - h_surf)
        zeta_a = antenna_z_phys - h_surf_0
        
        w0 = 2.0 / (self.k0 * np.sin(np.radians(beam_width_deg) / 2.0))
        tilt_rad = np.radians(tilt_deg)
        
        norm_factor = 1.0 / np.sqrt(w0 * np.sqrt(np.pi / 2.0))
        
        # 仅在物理空间 (0 到 nz-1) 初始化
        amp = norm_factor * np.exp(-((self.z - zeta_a)**2) / (w0**2))
        phase = np.exp(1j * self.k0 * self.z * np.sin(tilt_rad))
        
        self.u[:self.nz] = amp * phase
        self.u[self.nz:] = 0.0 # 镜像区置零

    def march(self, x_surf, h_surf, receiver_z_phys):
        """
        执行 SSFT 步进，并提取接收高度的场
        """
        steps = int(self.max_range / self.dx)
        
        # 用于记录对比数据
        results_x = []
        results_E_mag = []
        
        for s in range(steps):
            current_x = s * self.dx
            next_x = (s + 1) * self.dx
            
            # 1. 提取海面几何 (插值获取当前和下一步的高度)
            z_curr = np.interp(current_x, x_surf, h_surf)
            z_next = np.interp(next_x, x_surf, h_surf)
            
            # 计算倾角 beta
            slope = (z_next - z_curr) / self.dx
            beta = np.arctan(slope)
            
            # 2. 计算修正反射边界 (Image Method)
            geom_grazing = np.arctan(25.0 / (current_x + 1000.0))
            local_grazing = max(1e-6, geom_grazing + beta)
            gamma = self.calc_fresnel(local_grazing)
            
            # 3. 空间域处理 (Refraction)
            n2 = 1.0 # 假设标准大气 n=1
            val_ref = n2 - np.sin(beta)**2 + 0j # 巧妙利用 +0j 自动处理负数开根号
            refraction = np.exp(1j * self.k0 * self.dx * (np.sqrt(val_ref) - 1.0))
            
            # 作用于上半空间，并吸收
            self.u[:self.nz] = self.u[:self.nz] * refraction * self.absorber
            
            # 镜像对称映射 (u(-z) = gamma * u(z))
            # NumPy 数组切片非常优雅：u[nz+1:] 对应镜像空间，倒序排列对应物理空间的 1 到 nz-1
            self.u[self.nz+1:] = gamma * self.u[self.nz-1 : 0 : -1]
            self.u[0] *= (1.0 + gamma) # 边界点
            self.u[self.nz] = 0.0      # 最深处边界强制为0
            
            # 4. 傅里叶变换到波数域
            u_k = fft.fft(self.u)
            
            # 5. 波数域处理 (Diffraction)
            k_eff_sq = (self.k0 * np.cos(beta))**2
            val_diff = k_eff_sq - self.kz**2 + 0j # +0j 自动处理倏逝波的纯虚数衰减
            diffraction = np.exp(1j * self.dx * (np.sqrt(val_diff) - self.k0))
            
            u_k = u_k * diffraction
            
            # 6. 逆变换回空间域
            self.u = fft.ifft(u_k)  # SciPy ifft 自带 1/N 归一化！
            
            # ==========================================
            # 数据提取 (对应 FDTD)
            # ==========================================
            # 接收机在网格中的相对高度 zeta
            zeta_rx = receiver_z_phys - z_curr
            
            if 0 <= zeta_rx < self.max_z:
                # 插值获取精确高度的场强
                idx1 = int(zeta_rx / self.dz)
                idx2 = idx1 + 1 if idx1 + 1 < self.nz else idx1
                weight = (zeta_rx - idx1 * self.dz) / self.dz
                
                u_rx = self.u[idx1] * (1 - weight) + self.u[idx2] * weight
                
                # PE场 u 到 实际电场 E 的转换: E = u / sqrt(x)
                # 这与 2D FDTD 中点源的扩散衰减一致
                E_mag = np.abs(u_rx) / np.sqrt(max(current_x, 1.0))
            else:
                E_mag = 1e-12 # 淹没在海面下或超高
                
            results_x.append(current_x)
            results_E_mag.append(E_mag)
            
        return np.array(results_x), np.array(results_E_mag)

# ==========================================
# 运行比对实验
# ==========================================
if __name__ == "__main__":
    # --- 1. 参数设置 (必须与 FDTD 严格一致) ---
    FREQ_GHZ = 0.3
    DOMAIN_LEN = 190.0 # 对应 FDTD 中从源到右边界的距离 (-90 到 +100)
    
    # --- 2. 模拟海面数据 (实际应用中，这里应加载 FDTD 生成的 CSV) ---
    # 这里用简单的正弦波替代，验证时请替换为 JONSWAP 的 x 和 h 数组
    x_surf = np.linspace(0, DOMAIN_LEN, 2000)
    h_surf = 1.0 * np.sin(2 * np.pi * x_surf / 20.0) # 浪高1m, 波长20m
    
    # --- 3. 运行 PE 求解器 ---
    print("正在运行 PLST-PE 求解器...")
    pe = PLST_PE_Solver(freq_ghz=FREQ_GHZ, dx=0.5, dz=0.1, max_z=30.0, max_range=DOMAIN_LEN)
    
    # Tx在绝对高度5m，Rx在绝对高度5m (假设基准海面为0)
    h0 = h_surf[0]
    pe.init_gaussian_beam(antenna_z_phys=5.0, h_surf_0=h0, beam_width_deg=15.0)
    
    x_pe, E_pe = pe.march(x_surf, h_surf, receiver_z_phys=5.0)
    
    # --- 4. 绘图对比 ---
    plt.figure(figsize=(10, 5))
    
    # 转换为 dB 进行对比 (归一化幅值)
    E_pe_dB = 20 * np.log10(E_pe + 1e-12)
    plt.plot(x_pe, E_pe_dB, 'b-', linewidth=2, label='PLST-PE Model')
    
    # 尝试加载 FDTD 数据（如果你上一步导出了的话）
    try:
        fdtd_df = pd.read_csv("FDTD_Extraction_h5m.csv")
        # 对 FDTD 数据做适当对齐和平移
        E_fdtd = fdtd_df['Ez_Magnitude'].values
        # 匹配电场量级 (FDTD 源幅值可能与 PE 归一化不同，加一个常数偏移对其包络)
        offset_dB = E_pe_dB[10] - 20 * np.log10(E_fdtd[10] + 1e-12) 
        plt.plot(fdtd_df['Range_m'], 20 * np.log10(E_fdtd + 1e-12) + offset_dB, 
                 'r--', alpha=0.8, label='FDTD Ground Truth')
    except FileNotFoundError:
        print("未找到 FDTD 数据文件，仅显示 PE 结果。请在 FDTD 脚本中生成该文件。")
    
    plt.title('Validation: PLST-PE vs FDTD over Rough Sea (Receiver Height = 5m)')
    plt.xlabel('Distance from Transmitter (m)')
    plt.ylabel('Normalized Field Amplitude (dB)')
    plt.ylim([-80, 0])
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.savefig('PE_FDTD_Comparison.png', dpi=300)
    print("✅ 验证图已生成: PE_FDTD_Comparison.png")