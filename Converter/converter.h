#ifndef CONVERTER_H
#define CONVERTER_H

class Converter
{
public:
    enum class Mode{length, mass, temperature};
    enum class length_un{m, km, in, ft, mi};
    enum class mass_un{kg, lb, oz};
    enum class temp_un{C, F, K};

    Converter();

    double convert(
        double value,
        Mode mode,
        int fromUn,
        int toUn) const;

private:
    double lengthToBase(double value, length_un unit) const;
    double lengthFromBase(double value, length_un unit) const;
    double massToBase(double value, mass_un unit) const;
    double massFromBase(double value, mass_un unit) const;
    double tempToBase(double value, temp_un unit) const;
    double tempFromBase(double value, temp_un unit) const;
};

#endif // CONVERTER_H
