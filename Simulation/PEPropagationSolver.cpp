#include "PEPropagationSolver.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <cmath>
#include <memory>
#include <mutex>

#include <omp.h>
#include <spdlog/spdlog.h>

#include "Interface/TransferToFile.hpp"
#include "Simulation/PEModel.h"

PEPropagationSolver::PEPropagationSolver(ModelType modelType, const Fleet* fleet)
    : _fleet(fleet),
      _modelType(modelType) {}

LineMap PEPropagationSolver::compute1D(
    Transmitter_PE_data peData,
    EnvironmentData env,
    double receiverAntennaHeight) {
    AtmosphereModel atm(env.ductHeight);
    JONSWAPSurfaceGenerator surface(env.windSpeed);
    std::vector<double> n_profile(env.nz);
    for (int i = 0; i < env.nz; ++i) {
        const double z = i * env.dz;
        n_profile[i] = atm.getRefractiveIndex(z);
    }

    PEModel solver(peData.centralF_Ghz, env.dx, env.dz, env.nz);
    const double txSurfaceHeight = surface.getSurfaceHeight(peData.X_offset, peData.Y_offset, 0.0);
    solver.initializeGaussian(
        peData.antenna_height,
        txSurfaceHeight,
        peData.beamWidth_deg,
        peData.antennaPhi_deg);

    LineMap lossLine;
    for (double r = env.dx; r < env.maxRange; r += env.dx) {
        solver.step_PLST(r, n_profile, surface, 0);
        const double receiverSurfaceHeight = surface.getSurfaceHeight(r, 0.0, 0.0);
        const int rx_idx = static_cast<int>((receiverAntennaHeight - receiverSurfaceHeight) / env.dz);
        if (rx_idx < 0 || rx_idx >= env.nz) {
            lossLine.push_back(200.0);
            continue;
        }

        lossLine.push_back(solver.getPathLoss(rx_idx, r));
    }
    return lossLine;
}

double PEPropagationSolver::computePathLossAtRange(
    Transmitter_PE_data peData,
    EnvironmentData env,
    double receiverAntennaHeight,
    double targetRangeM) {
    if (targetRangeM < env.dx) {
        targetRangeM = env.dx;
    }

    const double originalMaxRange = env.maxRange;
    env.maxRange = std::max(originalMaxRange, targetRangeM + env.dx);
    const LineMap lossLine = compute1D(peData, env, receiverAntennaHeight);
    if (lossLine.empty()) {
        return 200.0;
    }

    const std::size_t index = static_cast<std::size_t>(
        std::max(0, static_cast<int>(std::ceil(targetRangeM / env.dx)) - 1));
    if (index >= lossLine.size()) {
        return lossLine.back();
    }
    return lossLine[index];
}

GridMatrix PEPropagationSolver::compute2D(
    Transmitter_PE_data peData,
    EnvironmentData env,
    double receiverAntennaHeight) {
    spdlog::info(
        "Starting 2D PE model computation for equipment: {}, on {}",
        peData.equipmenName,
        peData.shipName);

    const double map_width_m = env.maxRange;
    const double map_height_m = env.maxRange;
    const int grid_w = static_cast<int>(map_width_m / env.dx);
    const int grid_h = static_cast<int>(map_height_m / env.dx);
    const int num_angles = 360 / env.angleStepDeg;
    const int num_ranges = static_cast<int>(env.maxRange / env.dx);
    const double noise_floor = 200.0;

    GridMatrix output_grid = GridMatrix::Constant(grid_h, grid_w, noise_floor);
    const double inv_dx = 1.0 / env.dx;
    const double deg_to_idx = 1.0 / env.angleStepDeg;
    const double two_pi = 2.0 * M_PI;
    const double tx_x = peData.X_offset;
    const double tx_y = peData.Y_offset;

    AtmosphereModel atm(env.ductHeight);
    JONSWAPSurfaceGenerator surface(env.windSpeed);

    Eigen::MatrixXd polar_matrix(num_angles, num_ranges);
    polar_matrix.setConstant(noise_floor);

    const int max_threads = omp_get_max_threads();
    std::vector<std::unique_ptr<PEModel>> solvers;
    solvers.reserve(max_threads);
    static std::mutex fftw_mutex;
    for (int t = 0; t < max_threads; ++t) {
        std::lock_guard<std::mutex> lock(fftw_mutex);
        solvers.push_back(std::make_unique<PEModel>(peData.centralF_Ghz, env.dx, env.dz, env.nz));
    }

#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < num_angles; ++i) {
        const int tid = omp_get_thread_num();
        PEModel* solver = solvers[tid].get();

        const double az_deg = i * env.angleStepDeg;
        const double az_rad = az_deg * M_PI / 180.0;
        const double cos_az = std::cos(az_rad);
        const double sin_az = std::sin(az_rad);

        const double tx_h_surface = surface.getSurfaceHeight(peData.X_offset, peData.Y_offset, 0.0);
        solver->initializeGaussian(
            peData.antenna_height,
            tx_h_surface,
            peData.beamWidth_deg,
            peData.antennaPhi_deg);

        int range_idx = 0;
        for (double r = env.dx; r < env.maxRange && range_idx < num_ranges; r += env.dx) {
            solver->step_PLST(r, az_rad, atm, surface, 0.0);

            const double current_x_world = peData.X_offset + r * cos_az;
            const double current_y_world = peData.Y_offset + r * sin_az;
            const double h_r = surface.getSurfaceHeight(current_x_world, current_y_world, 0.0);
            const double zeta_rx = receiverAntennaHeight - h_r;
            const int rx_z_idx = static_cast<int>(zeta_rx / env.dz);

            if (rx_z_idx >= 0 && rx_z_idx < env.nz) {
                polar_matrix(i, range_idx) = solver->getPathLoss(rx_z_idx, r);
            } else {
                polar_matrix(i, range_idx) = noise_floor;
            }
            ++range_idx;
        }
    }

#pragma omp parallel for collapse(2)
    for (int y = 0; y < grid_h; ++y) {
        for (int x = 0; x < grid_w; ++x) {
            const double px_world = x * env.dx;
            const double py_world = y * env.dx;
            const double dx = px_world - tx_x;
            const double dy = py_world - tx_y;
            const double r = std::hypot(dx, dy);

            if (r >= env.maxRange || r < env.dx) {
                continue;
            }

            double theta = std::atan2(dy, dx);
            if (theta < 0.0) {
                theta += two_pi;
            }

            const double az_float = theta * 180.0 / M_PI * deg_to_idx;
            const double r_float = r * inv_dx;

            const int a0 = static_cast<int>(std::floor(az_float));
            const int a1 = (a0 + 1) % num_angles;
            const int r0 = static_cast<int>(std::floor(r_float));
            int r1 = r0 + 1;
            if (r1 >= num_ranges) {
                r1 = r0;
            }

            const double wa = az_float - a0;
            const double wr = r_float - r0;

            const double v00 = polar_matrix(a0, r0);
            const double v10 = polar_matrix(a1, r0);
            const double v01 = polar_matrix(a0, r1);
            const double v11 = polar_matrix(a1, r1);

            const double v_r0 = v00 * (1.0 - wa) + v10 * wa;
            const double v_r1 = v01 * (1.0 - wa) + v11 * wa;
            output_grid(y, x) = v_r0 * (1.0 - wr) + v_r1 * wr;
        }
    }

    return output_grid;
}
