#pragma once

namespace enh {

/// Compteur de la capsule de demonstration « Tester le ressort » (ENH-004, provisoire : retire
/// avec ENH-016). Regle metier unique : +1 par activation, sature a la valeur maximale d'un int
/// (jamais de depassement d'entier signe, qui serait un comportement indefini).
class DemoCounter {
public:
    DemoCounter() noexcept = default;
    /// Valeur de depart (tests, reprise) ; une valeur negative est ramenee a 0.
    explicit DemoCounter(int start) noexcept : count_(start < 0 ? 0 : start) {}

    [[nodiscard]] int count() const noexcept { return count_; }

    /// Incremente le compteur. Renvoie false (valeur inchangee) s'il est deja au maximum.
    bool increment() noexcept;

private:
    int count_ = 0;
};

} // namespace enh
