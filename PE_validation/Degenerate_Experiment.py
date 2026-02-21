import meep as mp
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import ImageGrid
import scipy.fft as fft
from abc import ABC, abstractmethod

# ==========================================
# 1. 物理对象层 (Scene Generator)
# ==========================================
class SceneGenerator:
    def __init__(self, terrain_type='flat', freq_ghz=0.3, lx=200.0, lz=30.0, dpml=2.0, 
                 tx_height=10.0, rx_height=10.0):
        self.terrain_type = terrain_type
        self.freq_ghz = freq_ghz
        self.lx = lx
        self.lz = lz
        self.dpml = dpml
        self.tx_height = tx_height
        self.rx_height = rx_height
        self.base_water_level = -5.0
        
        self.resolution = 20
        self.dx_fdtd = 1.0 / self.resolution
        self.x_full = np.arange(0, self.lx + 2 * self.dpml, self.dx_fdtd)
        self._generate_terrain()

    def _generate_terrain(self):
        """符合 OCP 原则的地形生成工厂"""
        if self.terrain_type == 'flat':
            # 退化验证：平坦海面
            self.h_full = np.zeros_like(self.x_full)
        elif self.terrain_type == 'gaussian_hill':
            # 规范地形验证：高斯山丘 (中心100m, 高度12m, 宽度15m，构造大斜率遮挡)
            hill_center = self.lx / 2.0
            hill_height = 12.0
            hill_width = 15.0
            self.h_full = hill_height * np.exp(-0.5 * ((self.x_full - hill_center) / hill_width)**2)
        else:
            raise ValueError("Unsupported terrain type")

# ==========================================
# 2. 求解器层 (Solvers)
# ==========================================
class TwoRaySolver:
    """双射线解析模型求解器 (针对 2D 柱面波传播)"""
    def __init__(self, scene: SceneGenerator):
        self.scene = scene
        self.k0 = 2 * np.pi * (scene.freq_ghz * 1e9) / 299792458.0

    def run(self, x_coords, z_coords):
        # 构建计算网格
        X, Z = np.meshgrid(x_coords, z_coords, indexing='ij')
        X_safe = np.maximum(X, 1e-3) # 防止除零
        
        # 直射径与反射径
        R1 = np.sqrt(X_safe**2 + (Z - self.scene.tx_height)**2)
        R2 = np.sqrt(X_safe**2 + (Z + self.scene.tx_height)**2)
        
        # 2D 柱面波解析解 (大参数 Hankel 函数渐近展开)，PEC 下边界反射系数为 -1
        E_2d = np.exp(1j * self.k0 * R1) / np.sqrt(R1) - np.exp(1j * self.k0 * R2) / np.sqrt(R2)
        E_2d_mag = np.abs(E_2d)
        
        return E_2d_mag

class FDTDSolver:
    """FDTD 全波求解器 (保持原样，省略部分重复代码以突出重点)"""
    def __init__(self, scene: SceneGenerator):
        self.scene = scene

    def run(self):
        mp.verbosity(0)
        x_center = np.mean(self.scene.x_full)
        x_meep = self.scene.x_full - x_center
        floor_z = -self.scene.lz/2 - self.scene.dpml
        
        sea_geometry = []
        for i in range(len(x_meep) - 1):
            z_val1 = min(self.scene.base_water_level + self.scene.h_full[i], self.scene.lz/2 - self.scene.dpml - 0.5)
            z_val2 = min(self.scene.base_water_level + self.scene.h_full[i+1], self.scene.lz/2 - self.scene.dpml - 0.5)
            v1, v2 = mp.Vector3(x_meep[i], floor_z), mp.Vector3(x_meep[i+1], floor_z)
            v3, v4 = mp.Vector3(x_meep[i+1], z_val2), mp.Vector3(x_meep[i], z_val1)
            sea_geometry.append(mp.Prism([v1, v2, v3, v4], height=mp.inf, material=mp.metal))
            
        cell_size = mp.Vector3(self.scene.lx + 2*self.scene.dpml, self.scene.lz + 2*self.scene.dpml)
        boundary_layers = [mp.Absorber(self.scene.dpml)]
        tx_physical_x = 10.0
        tx_z_meep = self.scene.base_water_level + self.scene.tx_height
        
        sources = [mp.Source(
            mp.ContinuousSource(frequency=self.scene.freq_ghz), component=mp.Ez,
            center=mp.Vector3(tx_physical_x - x_center, tx_z_meep), size=mp.Vector3(0, 0)
        )]
        
        sim = mp.Simulation(
            cell_size=cell_size, boundary_layers=boundary_layers,
            geometry=sea_geometry, sources=sources, resolution=self.scene.resolution,
            force_complex_fields=True
        )
        sim.run(until=self.scene.lx * 2.5)
        
        ez_data = sim.get_array(center=mp.Vector3(), size=cell_size, component=mp.Ez)
        
        z_coords = np.linspace(-cell_size.y/2, cell_size.y/2, ez_data.shape[1])
        x_coords = np.linspace(0, cell_size.x, ez_data.shape[0])
        tx_x_idx = np.argmin(np.abs(x_coords - tx_physical_x))
        
        fdtd_range = x_coords[tx_x_idx:] - x_coords[tx_x_idx]
        fdtd_2d_mag = np.abs(ez_data[tx_x_idx:, :])
        z_physical_coords = z_coords - self.scene.base_water_level
        
        return fdtd_range, fdtd_2d_mag, z_physical_coords, tx_physical_x

class PESolver:
    """抛物方程求解器 (带有低通点源初始化与 PLST 算子)"""
    def __init__(self, scene: SceneGenerator, dx=0.5, dz=0.1):
        self.c = 299792458.0
        self.freq = scene.freq_ghz * 1e9
        self.k0 = 2 * np.pi * self.freq / self.c
        self.dx, self.dz = dx, dz
        self.max_z = scene.lz
        self.nz = int(self.max_z / dz)
        self.fft_size = 2 * self.nz 
        self.z = np.arange(self.nz) * self.dz
        self.kz = fft.fftfreq(self.fft_size, d=self.dz) * 2 * np.pi
        self.u = np.zeros(self.fft_size, dtype=np.complex128)
        self.absorber = self._setup_absorber()

    def _setup_absorber(self):
        absorber = np.ones(self.nz)
        thick = int(self.nz * 0.25)
        absorber[-thick:] = 0.5 * (1 + np.cos(np.pi * np.arange(thick) / thick))
        return absorber

    def init_point_source(self, antenna_z_phys, h_surf_0):
        zeta_a = antenna_z_phys - h_surf_0
        idx = int(zeta_a / self.dz)
        if 0 <= idx < self.nz:
            self.u[idx] = 1.0 + 0j
        # 点源低通滤波防止高频发散
        kz_filter = np.exp(-(self.kz / (0.9 * self.k0))**10)
        self.u = fft.ifft(fft.fft(self.u) * kz_filter)

    def march(self, x_surf, h_surf, max_range):
        steps = int(max_range / self.dx)
        results_2d = []
        results_x = []
        
        for s in range(steps):
            current_x = s * self.dx
            z_curr = np.interp(current_x, x_surf, h_surf)
            z_next = np.interp((s + 1) * self.dx, x_surf, h_surf)
            
            beta = np.arctan((z_next - z_curr) / self.dx)
            val_ref = 1.0 - np.sin(beta)**2 + 0j 
            refraction = np.exp(1j * self.k0 * self.dx * (np.sqrt(val_ref) - 1.0))
            
            self.u[:self.nz] *= refraction * self.absorber
            self.u[self.nz+1:] = -1.0 * self.u[self.nz-1 : 0 : -1] # 镜像法 (PEC)
            self.u[self.nz] = 0.0 # 消除 Nyquist 极点残留
            self.u[0] *= (1.0 - 1.0) # 表面边界
            
            val_diff = (self.k0 * np.cos(beta))**2 - self.kz**2 + 0j 
            diffraction = np.exp(1j * self.dx * (np.sqrt(val_diff) - self.k0))
            self.u = fft.ifft(fft.fft(self.u) * diffraction)
            
            # 2D 物理场强 = u / sqrt(x)
            E_mag_2d = np.abs(self.u[:self.nz]) / np.sqrt(max(current_x, 1e-3))
            results_2d.append(E_mag_2d)
            results_x.append(current_x)
            
        return np.array(results_x), np.array(results_2d), self.z

# ==========================================
# 3. 数据评估层 (Metrics Evaluator)
# ==========================================
class MetricsEvaluator:
    @staticmethod
    def align_fields_2d(ref_2d_mag, test_2d_mag):
        """对全波解和PE/TwoRay进行全局dB偏置对齐"""
        ref_dB = 20 * np.log10(ref_2d_mag + 1e-12)
        test_dB = 20 * np.log10(test_2d_mag + 1e-12)
        
        # 使用中心区域计算偏置量
        valid_mask = (ref_dB > -60) & (test_dB > -60)
        if np.any(valid_mask):
            offset = np.mean(test_dB[valid_mask]) - np.mean(ref_dB[valid_mask])
        else:
            offset = 0.0
            
        return ref_dB + offset, test_dB

# ==========================================
# 4. 可视化层 (Visualization)
# ==========================================
class BaseVisualizer(ABC):
    @abstractmethod
    def plot(self, *args, **kwargs): pass

class HeatmapVisualizer(BaseVisualizer):
    def plot(self, ref_2d_dB, test_2d_dB, x_range, z_coords_ref, z_coords_test, 
             title_ref, title_test, filename, terrain_x=None, terrain_h=None):
        fig = plt.figure(figsize=(12, 8))
        
        # ImageGrid 共享单一 Colorbar
        grid = ImageGrid(fig, 111, nrows_ncols=(2, 1), axes_pad=0.4, 
                         share_all=True, cbar_location="right", cbar_mode="single", cbar_pad=0.1)
        
        vmin, vmax = -60, -10 # 统一能量映射范围
        
        # 子图 1: 基准图 (FDTD 或 Two-Ray)
        extent_ref = [x_range[0], x_range[-1], z_coords_ref[0], z_coords_ref[-1]]
        im1 = grid[0].imshow(ref_2d_dB.T, extent=extent_ref, origin='lower', aspect='auto', cmap='jet', vmin=vmin, vmax=vmax)
        grid[0].set_title(title_ref)
        grid[0].set_ylabel('Height (m)')
        
        # 子图 2: 测试图 (PE)
        extent_test = [x_range[0], x_range[-1], z_coords_test[0], z_coords_test[-1]]
        im2 = grid[1].imshow(test_2d_dB.T, extent=extent_test, origin='lower', aspect='auto', cmap='jet', vmin=vmin, vmax=vmax)
        grid[1].set_title(title_test)
        grid[1].set_xlabel('Range (m)')
        grid[1].set_ylabel('Height (m)')
        
        # 如果存在地形遮挡，填充黑色区域
        if terrain_x is not None and terrain_h is not None:
            grid[0].fill_between(terrain_x, 0, terrain_h, color='black')
            grid[1].fill_between(terrain_x, 0, terrain_h, color='black')

        # 添加全局唯一 Colorbar
        cbar = grid[0].cax.colorbar(im1)
        cbar.set_label('Field Strength (dB)')
        
        plt.savefig(filename, dpi=300)
        plt.close()

# ==========================================
# 5. 主流程 (执行两个规范场景验证)
# ==========================================
if __name__ == "__main__":
    
    # ---------------------------------------------------------
    # 场景 1: 退化验证 - 平坦海面 (PE vs Two-Ray 解析解)
    # ---------------------------------------------------------
    print("Running Case 1: Flat Surface Validation...")
    scene_flat = SceneGenerator(terrain_type='flat')
    
    pe_solver_flat = PESolver(scene_flat)
    pe_solver_flat.init_point_source(scene_flat.tx_height, 0.0)
    pe_range_flat, pe_2d_mag_flat, z_coords_pe_flat = pe_solver_flat.march(scene_flat.x_full, scene_flat.h_full, scene_flat.lx)
    
    # 使用 Two-Ray 提供精准 2D 场强
    tworay_solver = TwoRaySolver(scene_flat)
    tworay_2d_mag = tworay_solver.run(pe_range_flat, z_coords_pe_flat)
    
    ref_dB, pe_dB = MetricsEvaluator.align_fields_2d(tworay_2d_mag, pe_2d_mag_flat)
    
    HeatmapVisualizer().plot(
        ref_dB, pe_dB, pe_range_flat, z_coords_pe_flat, z_coords_pe_flat,
        title_ref='Analytical Two-Ray Model (Ground Truth)', 
        title_test='PLST-PE Model (Flat Degeneration)', 
        filename='Validation_Case1_FlatSurface.png'
    )
    
    # ---------------------------------------------------------
    # 场景 2: 标准地形验证 - 高斯山丘绕射 (PE vs FDTD 全波解)
    # ---------------------------------------------------------
    print("Running Case 2: Gaussian Hill Diffraction Validation...")
    scene_hill = SceneGenerator(terrain_type='gaussian_hill')
    
    # 运行 FDTD 捕捉真正的绕射、前向散射与遮挡阴影区
    fdtd_solver_hill = FDTDSolver(scene_hill)
    fdtd_range_hill, fdtd_2d_mag_hill, z_coords_fdtd_hill, tx_x_offset = fdtd_solver_hill.run()
    
    # 截取发射源右侧地形供 PE 使用
    tx_idx = np.argmin(np.abs(scene_hill.x_full - tx_x_offset))
    pe_x_input = scene_hill.x_full[tx_idx:] - scene_hill.x_full[tx_idx]
    pe_h_input = scene_hill.h_full[tx_idx:]
    
    # 运行 PE，观察 PLST 的大斜率相位补偿效果
    pe_solver_hill = PESolver(scene_hill)
    pe_solver_hill.init_point_source(scene_hill.tx_height, pe_h_input[0])
    pe_range_hill, pe_2d_mag_hill, z_coords_pe_hill = pe_solver_hill.march(pe_x_input, pe_h_input, fdtd_range_hill[-1])
    
    # 将 PE 结果插值/截取到与 FDTD 的距离网格匹配（粗略对齐用于画图）
   # 1. 在 Z 轴上对齐：将 PE 的相对高度映射到 FDTD 的绝对物理高度
    pe_2d_mapped_z = np.ones((len(pe_range_hill), len(z_coords_fdtd_hill))) * 1e-12
    for i in range(len(pe_range_hill)):
        z_abs_pe = z_coords_pe_hill + pe_h_input[i] # 加上地形高度恢复绝对坐标
        pe_2d_mapped_z[i, :] = np.interp(z_coords_fdtd_hill, z_abs_pe, pe_2d_mag_hill[i, :], left=1e-12, right=1e-12)
        
    # 2. 在 X 轴上对齐：插值到 FDTD 的距离网格
    pe_2d_mag_aligned = np.ones((len(fdtd_range_hill), len(z_coords_fdtd_hill))) * 1e-12
    for j in range(len(z_coords_fdtd_hill)):
        pe_2d_mag_aligned[:, j] = np.interp(fdtd_range_hill, pe_range_hill, pe_2d_mapped_z[:, j], left=1e-12, right=1e-12)
    
    ref_dB_hill, pe_dB_hill = MetricsEvaluator.align_fields_2d(fdtd_2d_mag_hill, pe_2d_mag_aligned)
    
    HeatmapVisualizer().plot(
        ref_dB_hill, pe_dB_hill, fdtd_range_hill, z_coords_fdtd_hill, z_coords_fdtd_hill, # 注意这里统一使用了 FDTD 的 Z 轴
        title_ref='FDTD Full-Wave Model (Diffraction & Shadowing)', 
        title_test='PLST-PE Model (Terrain Compensation)', 
        filename='Validation_Case2_GaussianHill.png',
        terrain_x=pe_x_input, terrain_h=pe_h_input
    )
    
    print("All canonical validations completed.")