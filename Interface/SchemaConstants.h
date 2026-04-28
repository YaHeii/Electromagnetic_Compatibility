#pragma once

// 统一维护标准 schema 的字段名、固定值和枚举字符串。
// 该头文件面向后续 JsonLoader、配置校验、模板生成和格式迁移复用。

namespace SchemaKeys {

inline constexpr char SchemaVersion[] = "schemaVersion";
inline constexpr char Environment[] = "environment";
inline constexpr char Usvs[] = "usvs";

inline constexpr char ID[] = "ID";
inline constexpr char Location[] = "location";
inline constexpr char Type[] = "type";
inline constexpr char Coordinates[] = "coordinates";
inline constexpr char Speed[] = "speed";
inline constexpr char ShipOrientationDeg[] = "shipOrientationDeg";
inline constexpr char Transmitters[] = "transmitters";
inline constexpr char Receivers[] = "receivers";

inline constexpr char MaxRange[] = "maxRange";
inline constexpr char DuctHeight[] = "ductHeight";
inline constexpr char WindSpeed[] = "windSpeed";
inline constexpr char Dx[] = "dx";
inline constexpr char Dz[] = "dz";
inline constexpr char Nz[] = "nz";
inline constexpr char AngleStepDeg[] = "angleStepDeg";

inline constexpr char GainDbi[] = "gainDbi";
inline constexpr char LocationOffset[] = "locationOffset";
inline constexpr char CenterFrequencyGHz[] = "centerFrequencyGHz";
inline constexpr char BandwidthMHz[] = "bandwidthMHz";
inline constexpr char PowerDbm[] = "powerDbm";
inline constexpr char AntennaPhiDeg[] = "antennaPhiDeg";
inline constexpr char BeamWidthDeg[] = "beamWidthDeg";
inline constexpr char Polarization[] = "polarization";
inline constexpr char AntennaType[] = "antennaType";

inline constexpr char SensitivityDbm[] = "sensitivityDbm";
inline constexpr char InterferenceMarginDb[] = "interferenceMarginDb";
inline constexpr char SinrMarginDb[] = "sinrMarginDb";
inline constexpr char NoiseFigureDb[] = "noiseFigureDb";

}  // namespace SchemaKeys

namespace SchemaValues {

inline constexpr char SchemaVersion_1_0_0[] = "1.0.0";
inline constexpr char Point3D[] = "Point3D";

inline constexpr char Transmitter[] = "TRANSMITTER";
inline constexpr char Receiver[] = "RECEIVER";

inline constexpr char Vertical[] = "VERTICAL";
inline constexpr char Horizontal[] = "HORIZONTAL";

inline constexpr char Omni[] = "OMNI";
inline constexpr char Directional[] = "DIRECTIONAL";
inline constexpr char Horn[] = "HORN";
inline constexpr char ShapedBeam[] = "SHAPED_BEAM";
inline constexpr char Reflector[] = "REFLECTOR";

}  // namespace SchemaValues

enum class SchemaDeviceType {
    Transmitter,
    Receiver
};

enum class SchemaPolarization {
    Vertical,
    Horizontal
};

enum class SchemaAntennaType {
    Omni,
    Directional,
    Horn,
    ShapedBeam,
    Reflector
};
