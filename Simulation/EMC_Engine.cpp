#include "Simulation/EMC_Engine.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <cmath>

#include <spdlog/spdlog.h>

#include "Interface/TransferToPEdata.hpp"
#include "Simulation/PEModel.h"
#include "Simulation/PEPropagationSolver.h"
#include "Utils/conversions.h"

namespace {

GridMap eigenToVector(const Eigen::MatrixXd& matrix) {
    GridMap values(static_cast<std::size_t>(matrix.rows()));

#pragma omp parallel for
    for (long row = 0; row < matrix.rows(); ++row) {
        values[static_cast<std::size_t>(row)].resize(static_cast<std::size_t>(matrix.cols()));
        Eigen::Map<Eigen::VectorXd>(
            values[static_cast<std::size_t>(row)].data(),
            matrix.cols()) = matrix.row(row);
    }
    return values;
}

ScalarField2D matrixToField(
    const QString& fieldId,
    const QString& displayName,
    ScalarFieldQuantity quantity,
    const QString& valueUnit,
    const Eigen::MatrixXd& matrix,
    double stepX,
    double stepY) {
    ScalarField2D field;
    field.fieldId = fieldId;
    field.displayName = displayName;
    field.quantity = quantity;
    field.valueUnit = valueUnit;
    field.axisXUnit = QStringLiteral("m");
    field.axisYUnit = QStringLiteral("m");
    field.rows = static_cast<int>(matrix.rows());
    field.cols = static_cast<int>(matrix.cols());
    field.originX = 0.0;
    field.originY = 0.0;
    field.stepX = stepX;
    field.stepY = stepY;
    field.values.reserve(static_cast<std::size_t>(field.rows * field.cols));

    for (int row = 0; row < field.rows; ++row) {
        for (int col = 0; col < field.cols; ++col) {
            field.values.push_back(matrix(row, col));
        }
    }
    return field;
}

EmitterResult makeEmitterResult(
    const Transmitter_PE_data& peData,
    const Eigen::MatrixXd& pathLoss,
    const EnvironmentData& environment) {
    EmitterResult result;
    result.emitterId = QString::fromStdString(peData.equipmenName);
    result.shipId = QString::fromStdString(peData.shipName);
    result.status = EmitterResultStatus::Succeeded;
    result.centerFrequencyGHz = peData.centralF_Ghz;
    result.transmitPowerDbm = peData.power_dbm;
    result.worldX = peData.X_offset;
    result.worldY = peData.Y_offset;
    result.worldZ = peData.Z_offset;
    result.field2D = matrixToField(
        QStringLiteral("path-loss-%1").arg(result.emitterId),
        QStringLiteral("Path Loss Field"),
        ScalarFieldQuantity::PathLossDb,
        QStringLiteral("dB"),
        pathLoss,
        environment.dx,
        environment.dx);
    return result;
}

GridMap scalarFieldToGridMap(const ScalarField2D& field) {
    if (field.rows <= 0 || field.cols <= 0 || field.values.empty()) {
        return {};
    }

    GridMap grid(static_cast<std::size_t>(field.rows));
    for (int row = 0; row < field.rows; ++row) {
        auto& rowValues = grid[static_cast<std::size_t>(row)];
        rowValues.resize(static_cast<std::size_t>(field.cols));
        for (int col = 0; col < field.cols; ++col) {
            const std::size_t index = static_cast<std::size_t>(row * field.cols + col);
            rowValues[static_cast<std::size_t>(col)] = field.values[index];
        }
    }
    return grid;
}

}  // namespace

EMC_Engine::EMC_Engine(ModelType modelType, std::unique_ptr<Fleet> fleet, DataSnapshot dataSnapshot)
    : _fleet(std::move(fleet)),
      _dataSnapshot(std::move(dataSnapshot)),
      _modelType(modelType) {
    if (!_fleet) {
        spdlog::error("EMC_Engine initialization failed: fleet is null");
    }
    _env = _dataSnapshot.environmentConfig;
    _propagationSolver = std::make_unique<PEPropagationSolver>(_modelType, _fleet.get());
}

EMC_Engine::~EMC_Engine() = default;

void EMC_Engine::markFailed(const QString& errorMessage) {
    _completedSuccessfully = false;
    _wasCancelled = false;
    _lastErrorMessage = errorMessage;
    _computationResult = {};
    _computationResult.status = SimulationResultStatus::Failed;
    _computationResult.errorMessage = errorMessage;
    spdlog::error("EMC_Engine failed: {}", errorMessage.toStdString());
}

void EMC_Engine::markCancelled() {
    _completedSuccessfully = false;
    _wasCancelled = true;
    _lastErrorMessage.clear();
    _computationResult = {};
    _computationResult.status = SimulationResultStatus::Cancelled;
    spdlog::info("EMC_Engine cancellation requested, aborting current task.");
}

void EMC_Engine::do_PE_computing() {
    _completedSuccessfully = false;
    _wasCancelled = false;
    _lastErrorMessage.clear();
    _computationResult = {};
    _computationResult.status = SimulationResultStatus::Failed;

    if (isStopRequested) {
        markCancelled();
        return;
    }
    if (!_fleet) {
        markFailed(QStringLiteral("Fleet 为空，无法启动仿真任务"));
        return;
    }
    if (!_propagationSolver) {
        markFailed(QStringLiteral("传播求解器未正确初始化"));
        return;
    }

    const std::vector<Transmitter_PE_data> peDataList = EquipmentConvertToMatrix(_fleet.get());
    if (peDataList.empty()) {
        markFailed(QStringLiteral("No transmitter data available in the frozen snapshot"));
        return;
    }

    const double receiverHeight = 25.0;
    const Eigen::MatrixXd firstLoss = _propagationSolver->compute2D(peDataList.front(), _env, receiverHeight);
    if (isStopRequested) {
        markCancelled();
        return;
    }

    Eigen::MatrixXd totalPowerMw =
        Eigen::MatrixXd::Zero(static_cast<int>(firstLoss.rows()), static_cast<int>(firstLoss.cols()));

    for (const auto& peData : peDataList) {
        if (isStopRequested) {
            markCancelled();
            return;
        }

        const Eigen::MatrixXd currentLoss = _propagationSolver->compute2D(peData, _env, receiverHeight);
        const Eigen::MatrixXd currentTxDbm = peData.power_dbm - currentLoss.array();
        accumulatePowerLinear(totalPowerMw, currentTxDbm);
        _computationResult.emitterResults.push_back(makeEmitterResult(peData, currentLoss, _env));
    }

    const Eigen::MatrixXd finalTotalDbm = totalPowerMw.unaryExpr([](double mw) {
        return mwToDbm(mw);
    });

    _computationResult.aggregatedField = matrixToField(
        QStringLiteral("aggregated-power"),
        QStringLiteral("Aggregated Power Field"),
        ScalarFieldQuantity::AggregatedPowerDbm,
        QStringLiteral("dBm"),
        finalTotalDbm,
        _env.dx,
        _env.dx);
    _computationResult.status = SimulationResultStatus::Succeeded;

    if (isStopRequested) {
        markCancelled();
        return;
    }

    std::ofstream out("PEcomputing_LinearAggregated.csv");
    spdlog::info("PEcomputing result will be saved in PEcomputing_LinearAggregated.csv");
    for (int row = 0; row < finalTotalDbm.rows(); ++row) {
        for (int col = 0; col < finalTotalDbm.cols(); ++col) {
            out << finalTotalDbm(row, col);
            if (col + 1 < finalTotalDbm.cols()) {
                out << ",";
            }
        }
        out << "\n";
    }

    _completedSuccessfully = true;
    emit peComputationFinished(scalarFieldToGridMap(_computationResult.aggregatedField));
}

GridMap EMC_Engine::do_PE_test() {
    spdlog::info("Starting PE test computation...");
    Transmitter_PE_data peData;
    peData.centralF_Ghz = 1;
    peData.antenna_height = 25.0;
    peData.beamWidth_deg = 2.0;
    peData.antennaPhi_deg = 0.0;
    _env.dx = 10.0;
    _env.dz = 0.1;
    _env.nz = 4096;
    _env.maxRange = 20000.0;
    _env.ductHeight = 20.0;
    _env.windSpeed = 7.0;

    return scalarFieldToGridMap(_computationResult.aggregatedField);
}

void EMC_Engine::do_Validation_TwoRay() {
    spdlog::info("Starting Level 1 Validation: Flat Sea & Standard Atmosphere...");

    Transmitter_PE_data data;
    data.centralF_Ghz = 1;
    data.antenna_height = 25.0;
    data.beamWidth_deg = 20.0;
    data.antennaPhi_deg = 0.0;
    _env.dx = 10.0;
    _env.dz = 0.1;
    _env.nz = 4096;
    _env.maxRange = 10000.0;
    _env.ductHeight = 0.0;
    _env.windSpeed = 0.001;

    PEModel solver(data.centralF_Ghz, _env.dx, _env.dz, _env.nz);
    std::ofstream out("validation_data.csv");
    out << "Range_m,PE_Loss_dB,Theory_Loss_dB\n";

    const double receiver_h = 15.0;
    for (double r = _env.dx; r < _env.maxRange; r += _env.dx) {
        const int rx_idx = static_cast<int>(receiver_h / _env.dz);
        const double pe_loss = solver.getPathLoss(rx_idx, r);

        const double lambda = 299792458.0 / (data.centralF_Ghz * 1.0e9);
        const double fspl = 20.0 * std::log10(4.0 * M_PI * r / lambda);
        const double delta_R = 2.0 * data.antenna_height * receiver_h / r;
        const double phase_diff = (2.0 * M_PI / lambda) * delta_R;
        const Complex E_total = 1.0 - std::exp(Complex(0, -phase_diff));
        const double F_linear = std::abs(E_total);
        const double theory_loss = fspl - 20.0 * std::log10(F_linear + 1e-10);

        out << r << "," << pe_loss << "," << theory_loss << "\n";
    }
    spdlog::info("Validation data saved to validation_data.csv");
}

void EMC_Engine::do_Validation_Roughness() {
    spdlog::info("Starting Level 2 Validation: PLST vs Miller-Brown...");

    Transmitter_PE_data data;
    data.centralF_Ghz = 10;
    data.antenna_height = 15.0;
    _env.maxRange = 20000.0;
    _env.dx = 10.0;
    _env.dz = 0.1;
    _env.nz = 2048;
    _env.windSpeed = 10.0;
    _env.ductHeight = 0.0;

    std::vector<double> n_profile(static_cast<std::size_t>(_env.nz), 1.0);
    PEModel solver_mb(data.centralF_Ghz, _env.dx, _env.dz, _env.nz);
    PEModel solver_plst(data.centralF_Ghz, _env.dx, _env.dz, _env.nz);

    std::ofstream out("validation_roughness.csv");
    out << "Range_m,Loss_MillerBrown,Loss_PLST\n";

    const double rx_h = 10.0;
    const int rx_idx = static_cast<int>(rx_h / _env.dz);
    JONSWAPSurfaceGenerator surface(_env.windSpeed);

    for (double r = _env.dx; r < _env.maxRange; r += _env.dx) {
        solver_mb.step_Miller_Brown(r, _env.windSpeed, n_profile);
        const double loss_mb = solver_mb.getPathLoss(rx_idx, r);

        solver_plst.step_PLST(r, n_profile, surface, 0.0);
        const double loss_plst = solver_plst.getPathLoss(rx_idx, r);

        out << r << "," << loss_mb << "," << loss_plst << "\n";
    }
}

void EMC_Engine::do_Validation_DuctLeakage() {
    spdlog::info("Starting Level 3 Validation: Duct Leakage Effect...");

    Transmitter_PE_data data;
    data.centralF_Ghz = 10;
    data.antenna_height = 10.0;
    _env.maxRange = 50000.0;
    _env.dx = 10.0;
    _env.dz = 0.1;
    _env.nz = 2048;
    _env.ductHeight = 20.0;

    const double wind_flat = 0.0;
    const double wind_rough = 15.0;

    AtmosphereModel atm(_env.ductHeight);
    std::vector<double> n_profile(static_cast<std::size_t>(_env.nz));
    for (int i = 0; i < _env.nz; ++i) {
        n_profile[static_cast<std::size_t>(i)] = atm.getRefractiveIndex(i * _env.dz);
    }

    JONSWAPSurfaceGenerator surf_flat(wind_flat);
    JONSWAPSurfaceGenerator surf_rough(wind_rough);

    PEModel solver_flat(data.centralF_Ghz, _env.dx, _env.dz, _env.nz);
    PEModel solver_rough(data.centralF_Ghz, _env.dx, _env.dz, _env.nz);

    const int steps = static_cast<int>(_env.maxRange / _env.dx);

    spdlog::info("Computing Flat Surface Case...");
    for (int s = 0; s < steps; ++s) {
        const double r = (s + 1) * _env.dx;
        solver_flat.step_PLST(r, n_profile, surf_flat, 0.0);
    }

    spdlog::info("Computing Rough Surface Case...");
    for (int s = 0; s < steps; ++s) {
        const double r = (s + 1) * _env.dx;
        solver_rough.step_PLST(r, n_profile, surf_rough, 0.0);
    }

    std::ofstream out("validation_leakage.csv");
    out << "Height_m,Loss_Flat_dB,Loss_Rough_dB\n";

    for (int i = 0; i < _env.nz; ++i) {
        const double z = i * _env.dz;
        const double loss_flat = solver_flat.getPathLoss(i, _env.maxRange);
        const double loss_rough = solver_rough.getPathLoss(i, _env.maxRange);
        if (z < 150.0) {
            out << z << "," << loss_flat << "," << loss_rough << "\n";
        }
    }
    spdlog::info("Validation data saved to validation_leakage.csv");
}
