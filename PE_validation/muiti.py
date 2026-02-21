import meep as mp
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import ImageGrid
import scipy.fft as fft
from abc import ABC, abstractmethod
# 仿真区间为300-500 
# ==========================================
# 1. 场景生成器 (Scene Generator)
# ==========================================
class SceneGenerator:
    # 修改 lx 为 1000.0，扩展仿真空间
    def __init__(self, freq_ghz=0.3, lx=500.0, lz=30.0, dpml=2.0, 
                 wind_speed=15.0, fetch_km=50.0, tx_height=5.0, rx_height=5.0):
        self.freq_ghz = freq_ghz
        self.lx = lx
        self.lz = lz
        self.dpml = dpml
        self.wind_speed = wind_speed
        self.fetch_km = fetch_km
        self.tx_height = tx_height
        self.rx_height = rx_height
        self.base_water_level = -5.0
        
        self.resolution = 20
        self.dx_fdtd = 1.0 / self.resolution
        self.x_full, self.h_full = self._generate_jonswap()

    def _generate_jonswap(self):
        g = 9.81              
        fetch_m = self.fetch_km * 1000.0
        X_tilde = (g * fetch_m) / (self.wind_speed**2)
        wp = 22 * (g / self.wind_speed) * (X_tilde**(-0.33))
        alpha = 0.076 * (X_tilde**(-0.22))
        
        length_m = self.lx + 2 * self.dpml
        k_min = 2 * np.pi / length_m
        k_max = 2 * np.pi / (2 * self.dx_fdtd)
        dk = 2 * np.pi / length_m
        k_arr = np.arange(k_min, k_max, dk)
        w_arr = np.sqrt(g * k_arr)
        
        S_pm = (alpha * g**2 / (w_arr**5)) * np.exp(-1.25 * (wp / w_arr)**4)
        gamma = 3.3  
        sigma = np.where(w_arr <= wp, 0.07, 0.09)
        r = np.exp(-(w_arr - wp)**2 / (2 * sigma**2 * wp**2))
        S_jonswap = S_pm * (gamma ** r)
        
        dw = np.diff(w_arr, prepend=w_arr[0])
        amplitudes = np.sqrt(2 * S_jonswap * dw)
        
        np.random.seed(42) 
        phases = np.random.uniform(0, 2*np.pi, size=len(k_arr))
        
        x = np.arange(0, length_m, self.dx_fdtd)
        h = np.zeros_like(x)
        for i in range(len(k_arr)):
            h += amplitudes[i] * np.cos(k_arr[i] * x + phases[i])
        return x, h

# ==========================================
# 2. 求解器层 (Solvers)
# ==========================================
class FDTDSolver:
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
        
        c_light = 299792458.0
        wavelength_m = c_light / (self.scene.freq_ghz * 1e9)
        freq_meep = 1.0 / wavelength_m
        
        sources = [mp.Source(
            mp.ContinuousSource(frequency=freq_meep),
            component=mp.Ez,
            center=mp.Vector3(tx_physical_x - x_center, tx_z_meep),
            size=mp.Vector3(0, 0)
        )]
        
        sim = mp.Simulation(
            cell_size=cell_size, boundary_layers=boundary_layers,
            geometry=sea_geometry, sources=sources, resolution=self.scene.resolution,
            force_complex_fields=True
        )
        sim.run(until=self.scene.lx * 2.5)
        
        # 提取数据
        ez_data = sim.get_array(center=mp.Vector3(), size=cell_size, component=mp.Ez)
        
        target_z_meep = self.scene.base_water_level + self.scene.rx_height
        z_coords = np.linspace(-cell_size.y/2, cell_size.y/2, ez_data.shape[1])
        z_idx = np.argmin(np.abs(z_coords - target_z_meep))
        
        x_coords = np.linspace(0, cell_size.x, ez_data.shape[0])
        tx_x_idx = np.argmin(np.abs(x_coords - tx_physical_x))
        
        fdtd_range = x_coords[tx_x_idx:] - x_coords[tx_x_idx]
        fdtd_1d_mag = np.abs(ez_data[tx_x_idx:, z_idx])
        fdtd_2d_mag = np.abs(ez_data[tx_x_idx:, :]) 
        
        z_physical_coords = z_coords - self.scene.base_water_level 
        
        return fdtd_range, fdtd_1d_mag, fdtd_2d_mag, z_physical_coords, tx_physical_x

class PESolver:
    def __init__(self, scene: SceneGenerator, dx=0.5, dz=0.1):
        self.c = 299792458.0
        self.freq = scene.freq_ghz * 1e9
        self.k0 = 2 * np.pi * self.freq / self.c
        self.dx = dx
        self.dz = dz
        self.max_z = scene.lz
        self.nz = int(self.max_z / dz)
        self.fft_size = 2 * self.nz 
        self.z = np.arange(self.nz) * self.dz
        self.kz = fft.fftfreq(self.fft_size, d=self.dz) * 2 * np.pi
        self.u = np.zeros(self.fft_size, dtype=np.complex128)
        self._setup_absorber()

    def _setup_absorber(self):
        self.absorber = np.ones(self.nz)
        absorb_layer_thickness = int(self.nz * 0.25)
        start_idx = self.nz - absorb_layer_thickness
        window = 0.5 * (1 + np.cos(np.pi * np.arange(absorb_layer_thickness) / absorb_layer_thickness))
        self.absorber[start_idx:] = window

    def init_point_source(self, antenna_z_phys, h_surf_0):
        zeta_a = antenna_z_phys - h_surf_0
        idx = int(zeta_a / self.dz)
        if 0 <= idx < self.nz:
            self.u[idx] = 1.0 + 0j
        
        sigma_kz = 0.4 * self.k0
        kz_filter = np.exp(-0.5 * (self.kz / sigma_kz)**2)
        self.u = fft.ifft(fft.fft(self.u) * kz_filter)

    def march(self, x_surf, h_surf, max_range, receiver_z_phys):
        steps = int(max_range / self.dx)
        results_x, results_E_mag = [], []
        results_2d = [] 
        h_surf_pe = []
        
        for s in range(steps):
            current_x = s * self.dx
            z_curr = np.interp(current_x, x_surf, h_surf)
            z_next = np.interp((s + 1) * self.dx, x_surf, h_surf)
            
            beta = np.arctan((z_next - z_curr) / self.dx)
            gamma = -1.0 + 0j
            
            val_ref = 1.0 - np.sin(beta)**2 + 0j 
            refraction = np.exp(1j * self.k0 * self.dx * (np.sqrt(val_ref) - 1.0))
            
            self.u[:self.nz] *= refraction * self.absorber
            self.u[self.nz+1:] = gamma * self.u[self.nz-1 : 0 : -1]
            self.u[self.nz] = 0.0  
            self.u[0] *= (1.0 + gamma) 
            
            u_k = fft.fft(self.u)
            val_diff = (self.k0 * np.cos(beta))**2 - self.kz**2 + 0j 
            diffraction = np.exp(1j * self.dx * (np.sqrt(val_diff) - self.k0))
            self.u = fft.ifft(u_k * diffraction)
            
            E_mag_2d = np.abs(self.u[:self.nz]) / np.sqrt(max(current_x, 1e-3))
            results_2d.append(E_mag_2d)
            h_surf_pe.append(z_curr) 
            
            zeta_rx = receiver_z_phys - z_curr
            if 0 <= zeta_rx < self.max_z:
                idx = int(zeta_rx / self.dz)
                E_mag = E_mag_2d[idx]
            else:
                E_mag = 1e-12 
                
            results_x.append(current_x)
            results_E_mag.append(E_mag)
            
        return np.array(results_x), np.array(results_E_mag), np.array(results_2d).T, self.z, np.array(h_surf_pe)

# ==========================================
# 3. 评估器与数学计算工具 (Metrics Evaluator)
# ==========================================
class MetricsEvaluator:
    @staticmethod
    def align_and_convert_to_dB(pe_mag, fdtd_mag, pe_range, fdtd_range):
        pe_dB = 20 * np.log10(pe_mag + 1e-12)
        fdtd_dB = 20 * np.log10(fdtd_mag + 1e-12)

        # 修改：将全局对齐区间更改为 100~1000m
        align_idx_pe = np.where((pe_range >= 300) & (pe_range <= 500))[0]
        align_idx_fdtd = np.where((fdtd_range >= 300) & (fdtd_range <= 500))[0]
        offset_dB = np.mean(pe_dB[align_idx_pe]) - np.mean(fdtd_dB[align_idx_fdtd])
        
        return pe_dB, fdtd_dB + offset_dB, offset_dB

    @staticmethod
    # 修改：min_range 默认改为 100.0
    def calc_rmse(pe_dB, fdtd_dB_aligned, pe_range, fdtd_range, min_range=300.0):
        pe_dB_interp = np.interp(fdtd_range, pe_range, pe_dB)
        valid_mask = fdtd_range >= min_range
        rmse = np.sqrt(np.mean((pe_dB_interp[valid_mask] - fdtd_dB_aligned[valid_mask])**2))
        return rmse

    @staticmethod
    # 修改：min_range 默认改为 100.0
    def calc_cumulative_rmse(pe_dB, fdtd_dB_aligned, pe_range, fdtd_range, min_range=100.0):
        pe_dB_interp = np.interp(fdtd_range, pe_range, pe_dB)
        cum_rmse = np.full_like(fdtd_range, np.nan)
        for i in range(len(fdtd_range)):
            if fdtd_range[i] >= min_range:
                valid_idx = np.where((fdtd_range >= min_range) & (fdtd_range <= fdtd_range[i]))[0]
                if len(valid_idx) > 0:
                    cum_rmse[i] = np.sqrt(np.mean((pe_dB_interp[valid_idx] - fdtd_dB_aligned[valid_idx])**2))
        return cum_rmse

# ==========================================
# 4. 可视化组件层 (Visualizers conforming to OCP)
# ==========================================
class BaseVisualizer(ABC):
    @abstractmethod
    def plot(self, *args, **kwargs):
        pass

class HeatmapVisualizer(BaseVisualizer):
    def plot(self, fdtd_2d_mag, pe_2d_mag_mapped, fdtd_range, pe_range, z_coords_fdtd, offset_dB):
        fig = plt.figure(figsize=(12, 8))
        grid = ImageGrid(fig, 111, nrows_ncols=(2, 1), axes_pad=0.4, 
                         share_all=True, cbar_location="right", cbar_mode="single", cbar_pad=0.1)
        
        fdtd_2d_dB = 20 * np.log10(fdtd_2d_mag + 1e-12) + offset_dB
        pe_2d_dB = 20 * np.log10(pe_2d_mag_mapped + 1e-12)
        vmin, vmax = -80, -20 
        
        extent_abs = [fdtd_range[0], fdtd_range[-1], z_coords_fdtd[0], z_coords_fdtd[-1]]
        
        im1 = grid[0].imshow(fdtd_2d_dB.T, extent=extent_abs, origin='lower', aspect='auto', cmap='jet', vmin=vmin, vmax=vmax)
        grid[0].set_title('FDTD 2D Field Distribution (Aligned)')
        grid[0].set_ylabel('Height (m)')
        grid[0].set_xlim([100, 1000]) # 限制 x 轴显示范围
        
        im2 = grid[1].imshow(pe_2d_dB, extent=extent_abs, origin='lower', aspect='auto', cmap='jet', vmin=vmin, vmax=vmax)
        grid[1].set_title('PE 2D Field Distribution (Mapped to Absolute Z)')
        grid[1].set_xlabel('Range (m)')
        grid[1].set_ylabel('Height (m)')
        grid[1].set_xlim([100, 1000]) # 限制 x 轴显示范围
        
        grid[0].cax.colorbar(im1)
        plt.savefig('Fig1_Heatmap_Comparison.png', dpi=300)
        plt.close()

class MultiWindSpeedVisualizer(BaseVisualizer):
    def plot(self, results_dict):
        plt.figure(figsize=(12, 8))
        colors = {5: 'g', 10: 'b', 15: 'r'}
        
        for ws, data in results_dict.items():
            fdtd_range, fdtd_dB, pe_range, pe_dB, rmse = data
            c = colors[ws]
            plt.plot(fdtd_range, fdtd_dB, c=c, linestyle='-', alpha=0.5, label=f'FDTD (WS={ws}m/s)')
            plt.plot(pe_range, pe_dB, c=c, linestyle='--', label=f'PE (WS={ws}m/s, RMSE={rmse:.2f}dB)')
            
        plt.title('Normalized Field Strength Comparison ([100, 1000]m)')
        plt.xlabel('Range (m)')
        plt.ylabel('Normalized Field Strength (dB)')
        plt.xlim([300, 500]) # 限制 x 轴显示范围
        plt.ylim([-90, -20])
        plt.legend(loc='lower left')
        plt.tight_layout()
        plt.savefig('Fig2_WindSpeed_Comparison.png', dpi=300)
        plt.close()

class CumulativeRMSEVisualizer(BaseVisualizer):
    def plot(self, results_dict):
        plt.figure(figsize=(10, 6))
        colors = {5: 'g', 10: 'b', 15: 'r'}
        
        for ws, data in results_dict.items():
            fdtd_range, fdtd_dB, pe_range, pe_dB, _ = data
            cum_rmse = MetricsEvaluator.calc_cumulative_rmse(pe_dB, fdtd_dB, pe_range, fdtd_range, min_range=100.0)
            
            # 为了绘图美观，将 np.nan 去除后再绘制
            valid_mask = ~np.isnan(cum_rmse)
            plt.plot(fdtd_range[valid_mask], cum_rmse[valid_mask], c=colors[ws], linewidth=2, label=f'Cum. RMSE (WS={ws}m/s)')
            
        plt.title('Cumulative RMSE vs. Range (>100m)')
        plt.xlabel('Range (m)')
        plt.ylabel('Cumulative RMSE (dB)')
        plt.xlim([100, 1000]) # 限制 x 轴显示范围
        plt.grid(True, linestyle=':', alpha=0.7)
        plt.legend()
        plt.tight_layout()
        plt.savefig('Fig3_Cumulative_RMSE.png', dpi=300)
        plt.close()

# ==========================================
# 5. 主控制程序 (执行多条件扩展测试)
# ==========================================
if __name__ == "__main__":
    wind_speeds = [5.0]
    multi_ws_results = {}
    
    heatmap_data = None

    for ws in wind_speeds:
        print(f"\n--- Running Simulation for Wind Speed = {ws} m/s ---")
        # lx=1000 会自动使用新的默认值
        scene = SceneGenerator(wind_speed=ws) 
        
        # 求解 FDTD
        fdtd_solver = FDTDSolver(scene)
        fdtd_range, fdtd_1d_mag, fdtd_2d_mag, z_coords_fdtd, tx_x_offset = fdtd_solver.run()
        
        # 提取 PE 输入海面
        tx_idx = np.argmin(np.abs(scene.x_full - tx_x_offset))
        pe_x_input = scene.x_full[tx_idx:] - scene.x_full[tx_idx]
        pe_h_input = scene.h_full[tx_idx:]
        
        # 求解 PE
        pe_solver = PESolver(scene)
        pe_solver.init_point_source(scene.tx_height, pe_h_input[0])
        pe_range, pe_1d_mag, pe_2d_mag_raw, z_coords_pe, h_surf_pe = pe_solver.march(pe_x_input, pe_h_input, fdtd_range[-1], scene.rx_height)
        
        # 坐标系逆映射重采样
        pe_2d_mag_mapped = np.ones((len(z_coords_fdtd), len(pe_range))) * 1e-12
        for i in range(len(pe_range)):
            z_abs_pe = z_coords_pe + h_surf_pe[i] 
            pe_2d_mag_mapped[:, i] = np.interp(z_coords_fdtd, z_abs_pe, pe_2d_mag_raw[:, i], left=1e-12, right=1e-12)
        
        # 数据评估计算 (内部包含 100m~1000m 的对齐和截断)
        pe_dB, fdtd_dB_aligned, offset_dB = MetricsEvaluator.align_and_convert_to_dB(pe_1d_mag, fdtd_1d_mag, pe_range, fdtd_range)
        rmse = MetricsEvaluator.calc_rmse(pe_dB, fdtd_dB_aligned, pe_range, fdtd_range)
        
        multi_ws_results[ws] = (fdtd_range, fdtd_dB_aligned, pe_range, pe_dB, rmse)
        
        if ws == 10.0:
            heatmap_data = (fdtd_2d_mag, pe_2d_mag_mapped, fdtd_range, pe_range, z_coords_fdtd, offset_dB)

    # ============================
    # 统一调用 Visualizer 进行绘制
    # ============================
    print("\nGenerating Plots...")
    if heatmap_data:
        HeatmapVisualizer().plot(*heatmap_data)
        
    MultiWindSpeedVisualizer().plot(multi_ws_results)
    CumulativeRMSEVisualizer().plot(multi_ws_results)
    
    print("All simulations and plotting completed. Check PNG files.")