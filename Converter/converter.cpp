#include "converter.h"

Converter::Converter() {}

double Converter::convert(double value, Mode mode, int fromUn, int toUn) const {
    switch (mode) {
    case Mode::length: {
        double base = lengthToBase(value, static_cast<length_un>(fromUn));
        return lengthFromBase(base, static_cast<length_un>(toUn));
    }
    case Mode::mass: {
        double base = massToBase(value, static_cast<mass_un>(fromUn));
        return massFromBase(base, static_cast<mass_un>(toUn));
    }
    case Mode::temperature: {
        double base = tempToBase(value, static_cast<temp_un>(fromUn));
        return tempFromBase(base, static_cast<temp_un>(toUn));
    }
    default:
        return value;
    }
}

double Converter::lengthToBase(double value, length_un unit) const {
    switch (unit) {
    case length_un::m:   return value;
    case length_un::km:  return value * 1000;
    case length_un::in:  return value * 0.0254;
    case length_un::ft:  return value * 0.3048;
    case length_un::mi:  return value * 1609.344;
    default:             return value;
    }
}

double Converter::lengthFromBase(double value, length_un unit) const {
    switch (unit) {
    case length_un::m:  return value;
    case length_un::km: return value / 1000.0;
    case length_un::in: return value * 39.37;
    case length_un::ft: return value * 3.281;
    case length_un::mi: return value / 1609.34;
    default: return value;
    }
}

double Converter::massToBase(double value, mass_un unit) const {
    switch (unit) {
    case mass_un::kg: return value;
    case mass_un::lb: return value * 0.453592;
    case mass_un::oz: return value * 0.0283495;
    default: return value;
    }
}

double Converter::massFromBase(double value, mass_un unit) const {
    switch (unit) {
    case mass_un::kg: return value;
    case mass_un::lb: return value / 0.453592;
    case mass_un::oz: return value / 0.0283495;
    default: return value;
    }
}

double Converter::tempToBase(double value, temp_un unit) const {
    switch (unit) {
    case temp_un::C: return value + 273.15;
    case temp_un::F: return (value - 32.0) * 5.0/9.0 + 273.15;
    case temp_un::K: return value;
    default: return value;
    }
}

double Converter::tempFromBase(double value, temp_un unit) const {
    switch (unit) {
    case temp_un::C: return value - 273.15;
    case temp_un::F: return (value - 273.15) * 9.0/5.0 + 32.0;
    case temp_un::K: return value;
    default: return value;
    }
}
