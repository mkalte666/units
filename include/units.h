#pragma once

#include <ratio>
#include <ctgmath>

#ifndef UNIT_BASETYPE
#define UNIT_BASETYPE double 
#endif 

#ifndef UNIT_NO_STREAMS 
#define UNIT_HAS_STREAMS
#endif 

#ifdef UNIT_HAS_STREAMS
#include <iostream>
#include <sstream>
#endif 

namespace unit {

template <typename Unit, typename MagnitudeRepresentation = UNIT_BASETYPE>
class Quantity {
    template <typename, typename>
    friend class Quantity;

   public:
    using unit = Unit;
    using magnitude_type = MagnitudeRepresentation;
    using classtype = Quantity<unit, magnitude_type>;

    constexpr explicit Quantity() : m_magnitude(0) {}
    constexpr explicit Quantity(magnitude_type const& v) : m_magnitude(v) {}

    constexpr Quantity(classtype const& v) = default;

    template <typename R>
    constexpr Quantity(Quantity<unit, R> const& v) : m_magnitude(v.m_magnitude) {}

    classtype& operator=(classtype const& v) = default;

    constexpr magnitude_type const& magnitude() const { return m_magnitude; }

   private:
    template <typename U, typename L, typename R>
    friend constexpr Quantity<U, L>& operator+=(Quantity<U, L>&, Quantity<U, R> const&);

    template <typename U, typename L, typename R>
    friend constexpr Quantity<U, L>& operator-=(Quantity<U, L>&, Quantity<U, R> const&);

    template <typename U, typename L, typename R>
    friend constexpr Quantity<U, L>& operator*=(Quantity<U, L>&, Quantity<U, R> const&);

    template <typename U, typename L, typename R>
    friend constexpr Quantity<U, L>& operator/=(Quantity<U, L>&, Quantity<U, R> const&);

    magnitude_type m_magnitude;
};

using TExponent = int;
using ExponentIndex = int;
using DimensionIndex = int;

template <TExponent... dimensionExponents>
struct Unit;

namespace helper {


template <typename Unit, ExponentIndex i>
constexpr TExponent exponent() {
    return Unit::template exponent_value<i>();
}

namespace nthElement {

using Index = unsigned int;

template <Index n, TExponent Arg, TExponent... Args>
struct NthElement {
    static constexpr int value() { return NthElement<n - 1, Args...>::value(); }
};

template <TExponent Arg, TExponent... Args>
struct NthElement<0, Arg, Args...> {
    static constexpr int value() { return Arg; }
};
}

namespace baseunit {

template <DimensionIndex it, DimensionIndex index, TExponent... exponents>
struct BaseUnitGen {
    using type = typename BaseUnitGen<it - 1, index, it == index ? 1 : 0, exponents...>::type;
};

template <DimensionIndex index, TExponent... exponents>
struct BaseUnitGen<-1, index, exponents...> {
    using type = Unit<exponents...>;
};
}
}

template <int dimension, int lastDimension>
using BaseUnit = typename helper::baseunit::BaseUnitGen<lastDimension, dimension>::type;

template<TExponent ... dimensionExponents >
struct Unit{

    using classtype = Unit<dimensionExponents...>;

    static constexpr unsigned exponent_count() {return sizeof...(dimensionExponents);}

    template<ExponentIndex i>
    static constexpr TExponent exponent_value() { return helper::nthElement::NthElement<i,dimensionExponents...>::value(); }
};


namespace helper {

namespace op {

template <typename U1, typename U2, int index>
struct sum {
    static constexpr int value() { return U1::template exponent_value<index>() + U2::template exponent_value<index>(); }
};

template <typename U1, typename U2, int index>
struct difference {
    static constexpr int value() { return U1::template exponent_value<index>() - U2::template exponent_value<index>(); }
};

template <typename U1, typename fract, int index>
struct multiply {
    static_assert(helper::exponent<U1, index>() * fract::num % fract::den == 0,
                  "rasing a unit to a fractional-power is not allowed");
    static constexpr int value() { return helper::exponent<U1, index>() * fract::num / fract::den; }
};

template <ExponentIndex it, typename U1, typename U2, template <class, class, int> class Op, TExponent... exponents>
struct UnitOperator {
    using type = typename UnitOperator<it - 1, U1, U2, Op, Op<U1, U2, it>::value(), exponents...>::type;
};

template <typename U1, typename U2, template <class, class, int> class Op, TExponent... exponents>
struct UnitOperator<-1, U1, U2, Op, exponents...> {
    using type = Unit<exponents...>;
};
}
}

template <typename U1, typename U2>
using product_unit = typename helper::op::UnitOperator<U1::exponent_count() - 1, U1, U2, helper::op::sum>::type;

template <typename U1, typename U2>
using quotient_unit = typename helper::op::UnitOperator<U1::exponent_count() - 1, U1, U2, helper::op::difference>::type;

template <typename U1, typename fract>
using raised_unit = typename helper::op::UnitOperator<U1::exponent_count() - 1, U1, fract, helper::op::multiply>::type;

template <typename U>
using sqare_unit = product_unit<U, U>;




template <typename TDst, typename QSrc>
constexpr Quantity<typename QSrc::unit, TDst> static_unit_cast(QSrc const& l) {
    return Quantity<typename QSrc::unit, TDst>{static_cast<TDst>(l.magnitude())};
}

template <typename U, typename LT, typename RT>
constexpr Quantity<U, LT>& operator+=(Quantity<U, LT>& l, Quantity<U, RT> const& r) {
    return l.m_magnitude += r.m_magnitude, l;
}

template <typename U, typename LT, typename RT>
constexpr Quantity<U, LT>& operator-=(Quantity<U, LT>& l, Quantity<U, RT> const& r) {
    return l.m_magnitude -= r.m_magnitude, l;
}

template <typename U, typename LT, typename RT>
constexpr Quantity<U, LT>& operator*=(Quantity<U, LT>& l, Quantity<U, RT> const& r) {
    return l.m_magnitude *= r.m_magnitude, l;
}

template <typename U, typename LT, typename RT>
constexpr Quantity<U, LT>& operator/=(Quantity<U, LT>& l, Quantity<U, RT> const& r) {
    return l.m_magnitude /= r.m_magnitude, l;
}

template <typename LU, typename LT, typename RU, typename RT>
constexpr auto operator*(Quantity<LU, LT> const& l, Quantity<RU, RT> const& r)
    -> Quantity<product_unit<LU, RU>, decltype(l.magnitude() * r.magnitude())> {
    return Quantity<product_unit<LU, RU>, decltype(l.magnitude() * r.magnitude())>{l.magnitude() * r.magnitude()};
}

template <typename LU, typename LT, typename RU, typename RT>
constexpr auto operator/(Quantity<LU, LT> const& l, Quantity<RU, RT> const& r)
    -> Quantity<quotient_unit<LU, RU>, decltype(l.magnitude() / r.magnitude())> {
    return Quantity<quotient_unit<LU, RU>, decltype(l.magnitude() / r.magnitude())>{l.magnitude() / r.magnitude()};
}

template <typename U, typename LT, typename RT>
constexpr auto operator+(Quantity<U, LT> const& l, Quantity<U, RT> const& r)
    -> Quantity<U, decltype(l.magnitude() + r.magnitude())> {
    return Quantity<U, decltype(l.magnitude() + r.magnitude())>{l.magnitude() + r.magnitude()};
}

template <typename U, typename LT, typename RT>
constexpr auto operator-(Quantity<U, LT> const& l, Quantity<U, RT> const& r)
    -> Quantity<U, decltype(l.magnitude() - r.magnitude())> {
    return Quantity<U, decltype(l.magnitude() - r.magnitude())>{l.magnitude() - r.magnitude()};
}

template <typename U, typename LT, typename RT>
constexpr bool operator<(Quantity<U, LT> const& l, Quantity<U, RT> const& r) {
    return l.magnitude() < r.magnitude();
}

template <typename U, typename LT, typename RT>
constexpr bool operator<=(Quantity<U, LT> const& l, Quantity<U, RT> const& r) {
    return l.magnitude() <= r.magnitude();
}

template <typename U, typename LT, typename RT>
constexpr bool operator>(Quantity<U, LT> const& l, Quantity<U, RT> const& r) {
    return l.magnitude() > r.magnitude();
}

template <typename U, typename LT, typename RT>
constexpr bool operator>=(Quantity<U, LT> const& l, Quantity<U, RT> const& r) {
    return l.magnitude() >= r.magnitude();
}

template <typename U, typename LT, typename RT>
constexpr bool operator==(Quantity<U, LT> const& l, Quantity<U, RT> const& r) {
    return l.magnitude() == r.magnitude();
}

template <typename U, typename LT, typename RT>
constexpr bool operator!=(Quantity<U, LT> const& l, Quantity<U, RT> const& r) {
    return l.magnitude() != r.magnitude();
}

template <typename LU, typename LT>
constexpr Quantity<LU, LT> operator-(Quantity<LU, LT> const& l) {
    return Quantity<LU, LT>{-l.magnitude()};
}


template <typename U, typename T>
constexpr auto
abs(Quantity<U, T> const& q)
        -> Quantity<U, decltype(abs(q.magnitude()))>
{
    using std::abs;
    return Quantity<U, decltype(abs(q.magnitude()))>{
                                abs(q.magnitude())};
}

template <typename U, typename T>
constexpr auto
sqrt(Quantity<U, T> const& q)
        -> Quantity<raised_unit<U, std::ratio<1, 2>>, decltype(sqrt(q.magnitude()))>
{
    using std::sqrt;
    return Quantity<raised_unit<U, std::ratio<1, 2>>, decltype(sqrt(q.magnitude()))>{
                                                               sqrt(q.magnitude())};
}

namespace helper {

template <typename power, typename T>
constexpr auto
pow_impl(T const& v)
 -> decltype(pow(v, static_cast<UNIT_BASETYPE>(power::num) / static_cast<UNIT_BASETYPE>(power::den)))

{
    using std::pow;
    return pow(v, static_cast<UNIT_BASETYPE>(power::num) / static_cast<UNIT_BASETYPE>(power::den));
}

}

template <typename power, typename U, typename T>
constexpr auto
pow(Quantity<U, T> const& q)
        -> Quantity<raised_unit<U, power>, decltype(helper::pow_impl<power, T>(q.magnitude()))>
{
    return Quantity<raised_unit<U, power>, decltype(helper::pow_impl<power, T>(q.magnitude()))>{
                                                    helper::pow_impl<power, T>(q.magnitude())};
}

template <typename U, typename T>
constexpr auto
cube(Quantity<U, T> const& q)
        -> Quantity<product_unit<product_unit<U, U>, U>, decltype((q.magnitude() * q.magnitude()) * q.magnitude())>
{
    return Quantity<product_unit<product_unit<U, U>, U>, decltype((q.magnitude() * q.magnitude()) * q.magnitude())>{
                                                                  (q.magnitude() * q.magnitude()) * q.magnitude()};
}

template <typename U, typename T>
constexpr auto
square(Quantity<U, T> const& q)
        -> Quantity<product_unit<U, U>, decltype(q.magnitude() * q.magnitude())>
{
    return Quantity<product_unit<U, U>, decltype(q.magnitude() * q.magnitude())>{
                                                 q.magnitude() * q.magnitude()};
}





template <typename U>
constexpr const char* unitSymbol();

namespace helper {
#ifdef UNIT_HAS_STREAMS
namespace print {

template <typename U, DimensionIndex pos>
void print_unit_symbol_and_exponent(std::ostream& s) {
    static constexpr TExponent e = helper::exponent<U, pos>();
    if (e == 0)
        return;
    s << unitSymbol<typename baseunit::BaseUnitGen<U::exponent_count() - 1, pos>::type>();
    if (e != 1) {
        s << "^" << e;
    }
}

template <typename U, DimensionIndex pos>
struct DimensionsPrinter {
    static void print_unit(std::ostream& s) {
        DimensionsPrinter<U, pos - 1>::print_unit(s);
        print_unit_symbol_and_exponent<U, pos>(s);
    }
};

template <typename U>
struct DimensionsPrinter<U, 0> {
    static void print_unit(std::ostream& s) { print_unit_symbol_and_exponent<U, 0>(s); }
};
}
#endif //UNIT_HAS_STREAMS
}


#ifdef UNIT_HAS_STREAMS
template <typename U>
void print_unit(std::ostream& s) {
    helper::print::DimensionsPrinter<U, U::exponent_count() - 1>::print_unit(s);
}

template <typename Unit>
std::istream& read_unit(std::istream& s) {
    std::ostringstream expectedBuff;
    print_unit<Unit>(expectedBuff);

    for (auto const& expected : expectedBuff.str()) {
        char found;
        s >> found;
        if (!s || found != expected) {
            s.setstate(std::ios::failbit);
            return s;
        }
    }
    return s;
}

template <typename Unit, typename TValue>
std::istream& operator>>(std::istream& s, Quantity<Unit, TValue>& v) {
    TValue val;
    s >> val;
    if (!s)
        return s;

    if (!read_unit<Unit>(s))
        return s;

     v = Quantity<Unit, TValue>{val};
    return s;
}

template <typename Unit, typename TValue>
std::ostream& operator<<(std::ostream& s, Quantity<Unit, TValue> const& v) {
    s << v.magnitude();
    print_unit<Unit>(s);
    return s;
}

#endif //UNIT_HAS_STREAMS

namespace helper {

template <typename ratioIn, typename ratioOut, typename T>
constexpr T rescale(T const& v) {
    using r = std::ratio_divide<ratioIn, ratioOut>;
    return v * T{r::num} / T{r::den};
}

template <typename ratioIn, typename T>
constexpr T rescaleTo1(T const& v) {
    return rescale<ratioIn, std::ratio<1>, T>(v);
}
}

template<typename T> constexpr T femto(T const& v) { return helper::rescaleTo1<std::femto>(v); }
template<typename T> constexpr T pico(T const& v) { return helper::rescaleTo1<std::pico>(v); }
template<typename T> constexpr T nano(T const& v) { return helper::rescaleTo1<std::nano>(v); }
template<typename T> constexpr T micro(T const& v) { return helper::rescaleTo1<std::micro>(v); }
template<typename T> constexpr T milli(T const& v) { return helper::rescaleTo1<std::milli>(v); }
template<typename T> constexpr T centi(T const& v) { return helper::rescaleTo1<std::centi>(v); }
template<typename T> constexpr T deci(T const& v) { return helper::rescaleTo1<std::deci>(v); }
template<typename T> constexpr T deca(T const& v) { return helper::rescaleTo1<std::deca>(v); }
template<typename T> constexpr T hecto(T const& v) { return helper::rescaleTo1<std::hecto>(v); }
template<typename T> constexpr T kilo(T const& v) { return helper::rescaleTo1<std::kilo>(v); }
template<typename T> constexpr T mega(T const& v) { return helper::rescaleTo1<std::mega>(v); }
template<typename T> constexpr T giga(T const& v) { return helper::rescaleTo1<std::giga>(v); }
template<typename T> constexpr T tera(T const& v) { return helper::rescaleTo1<std::tera>(v); }
template<typename T> constexpr T peta(T const& v) { return helper::rescaleTo1<std::peta>(v); }


namespace u {




using unitless = BaseUnit<-1,6>;

using meter = BaseUnit<0,6>;
using kilogram = BaseUnit<1,6>;
using second = BaseUnit<2,6>;
using ampere = BaseUnit<3,6>;
using kelvin = BaseUnit<4,6>;
using mole = BaseUnit<5,6>;
using candela = BaseUnit<6,6>;




using radian = unitless;
using steradian = unitless;
using hertz = quotient_unit< unitless, second>;
using newton = quotient_unit< product_unit<kilogram, meter>, sqare_unit<second>>;
using pascal = quotient_unit< newton, sqare_unit<meter>>;
using joule = quotient_unit< newton, meter>;
using watt = quotient_unit< joule, second>;
using coulomb = product_unit< second, ampere>;
using volt = quotient_unit< watt, ampere>;
using farad = quotient_unit< coulomb, volt>;
using ohm = quotient_unit< volt, ampere>;
using siemens = quotient_unit< ampere, volt>;
using weber = product_unit< volt, second>;
using tesla = quotient_unit< weber, sqare_unit<meter>>;
using henry = quotient_unit< weber, ampere>;
using lumen = product_unit< candela, steradian>;
using lux = quotient_unit< lumen, sqare_unit<meter>>;
using becquerel = hertz;
using gray = quotient_unit< joule, kilogram>;
using sievert = gray;
using katal = quotient_unit< mole, second>;



using meter_cubed = product_unit< meter, sqare_unit<meter>>;
using meter_per_second = quotient_unit< meter, second>;
using meter_per_second_squared = quotient_unit< meter, sqare_unit<second>>;

using mol_inv = quotient_unit< unitless, mole>;

}

namespace t {

    using def = UNIT_BASETYPE;


    using unitless = Quantity<u::unitless, def>;
    using meter = Quantity<u::meter, def>;
    using kilogram = Quantity<u::kilogram, def>;
    using second = Quantity<u::second, def>;
    using ampere = Quantity<u::ampere, def>;
    using kelvin = Quantity<u::kelvin, def>;
    using mole = Quantity<u::mole, def>;
    using candela = Quantity<u::candela, def>;


    using radian = Quantity<u::radian, def>;
    using steradian = Quantity<u::steradian, def>;
    using hertz = Quantity<u::hertz, def>;
    using newton = Quantity<u::newton, def>;
    using pascal = Quantity<u::pascal, def>;
    using joule = Quantity<u::joule, def>;
    using watt = Quantity<u::watt, def>;
    using coulomb = Quantity<u::coulomb, def>;
    using volt = Quantity<u::volt, def>;
    using farad = Quantity<u::farad, def>;
    using ohm = Quantity<u::ohm, def>;
    using siemens = Quantity<u::siemens, def>;
    using weber = Quantity<u::weber, def>;
    using tesla = Quantity<u::tesla, def>;
    using henry = Quantity<u::henry, def>;
    using lumen = Quantity<u::lumen, def>;
    using lux = Quantity<u::lux, def>;
    using becquerel = Quantity<u::becquerel, def>;
    using gray = Quantity<u::gray, def>;
    using sievert = Quantity<u::sievert, def>;
    using katal = Quantity<u::katal, def>;


    using meter_cubed = Quantity<u::meter_cubed, def>;
}


constexpr t::unitless unitless{1};
constexpr t::unitless number{1};
constexpr t::meter meter{1};
constexpr t::kilogram kilogram{1};
constexpr t::second second{1};
constexpr t::ampere ampere{1};
constexpr t::kelvin kelvin{1};
constexpr t::mole mole{1};
constexpr t::candela candela{1};


constexpr t::radian radian{1};
constexpr t::steradian steradian{1};
constexpr t::hertz hertz{1};
constexpr t::newton newton{1};
constexpr t::pascal pascal{1};
constexpr t::joule joule{1};
constexpr t::watt watt{1};
constexpr t::coulomb coulomb{1};
constexpr t::volt volt{1};
constexpr t::farad farad{1};
constexpr t::ohm ohm{1};
constexpr t::siemens siemens{1};
constexpr t::weber weber{1};
constexpr t::tesla tesla{1};
constexpr t::henry henry{1};
constexpr t::lumen lumen{1};
constexpr t::lux lux{1};
constexpr t::becquerel becquerel{1};
constexpr t::gray gray{1};
constexpr t::sievert sievert{1};
constexpr t::katal katal{1};


constexpr t::meter inch{centi(2.54)};
constexpr t::meter food{0.3048};
constexpr t::meter mile{1609.344};
constexpr t::meter yard{0.9144};
constexpr t::kilogram pound{0.45359237};
constexpr t::kilogram ounce{0.02834952};
constexpr t::kilogram gram{helper::rescale<std::ratio<1>,std::kilo>(1.0)};
constexpr t::second minute{60};
constexpr t::second hour{60*60};
constexpr t::second day{60*60*24};
constexpr t::joule calorie{4.184};
constexpr t::joule watt_hour{3600};
constexpr t::meter_cubed liter{0.001};
constexpr t::meter_cubed gallon{3.785412 * 0.001};
constexpr t::pascal bar{kilo(100)};
constexpr t::unitless parts_per_million{micro(1.0)};
constexpr t::unitless percent{centi(1.0)};





constexpr t::kelvin celsius ( UNIT_BASETYPE v ){ return t::kelvin {static_cast<UNIT_BASETYPE>(v+273.15)};}
constexpr t::kelvin fahrenheit ( UNIT_BASETYPE v ){ return t::kelvin {static_cast<UNIT_BASETYPE>((v+459.67)*5.0/9.0)};}


namespace literals {

constexpr t::unitless operator"" _unitless ( long double v ) {return t::unitless {static_cast<UNIT_BASETYPE>(v)};}
constexpr t::unitless operator"" _number ( long double v ) {return t::unitless {static_cast<UNIT_BASETYPE>(v)};}
constexpr t::unitless operator"" _n ( long double v ) {return t::unitless {static_cast<UNIT_BASETYPE>(v)};}
constexpr t::newton operator"" _newton ( long double v ) {return t::newton {static_cast<UNIT_BASETYPE>(v)};}

}


template<typename U> constexpr const char* unitSymbol();
template<> inline constexpr const char* unitSymbol< u::meter>(){ return "m";}
template<> inline constexpr const char* unitSymbol<u::kilogram>(){ return "kg";}
template<> inline constexpr const char* unitSymbol< u::second>(){ return "s";}
template<> inline constexpr const char* unitSymbol< u::ampere>(){ return "A";}
template<> inline constexpr const char* unitSymbol< u::kelvin>(){ return "K";}
template<> inline constexpr const char* unitSymbol< u::mole>(){ return "mol";}
template<> inline constexpr const char* unitSymbol< u::candela>(){ return "cd";}


#ifdef UNIT_HAS_STREAMS
template<typename U> void print_unit(std::ostream& s);


template<> inline void print_unit< u::hertz>(std::ostream& s){ s<<"Hz"; }
template<> inline void print_unit< u::newton>(std::ostream& s){ s<<"N"; }
template<> inline void print_unit< u::pascal>(std::ostream& s){ s<<"Pa"; }
template<> inline void print_unit< u::joule>(std::ostream& s){ s<<"J"; }
template<> inline void print_unit< u::watt>(std::ostream& s){ s<<"W"; }
template<> inline void print_unit< u::coulomb>(std::ostream& s){ s<<"C"; }
template<> inline void print_unit< u::volt>(std::ostream& s){ s<<"V"; }
template<> inline void print_unit< u::farad>(std::ostream& s){ s<<"F"; }
template<> inline void print_unit< u::ohm>(std::ostream& s){ s<<"Ohm"; }
template<> inline void print_unit< u::siemens>(std::ostream& s){ s<<"S"; }
template<> inline void print_unit< u::weber>(std::ostream& s){ s<<"Wb"; }
template<> inline void print_unit< u::tesla>(std::ostream& s){ s<<"T"; }
template<> inline void print_unit< u::henry>(std::ostream& s){ s<<"H"; }
template<> inline void print_unit< u::lumen>(std::ostream& s){ s<<"lm"; }
template<> inline void print_unit< u::lux>(std::ostream& s){ s<<"Lx"; }

template<> inline void print_unit< u::gray>(std::ostream& s){ s<<"Gy"; }

template<> inline void print_unit< u::katal>(std::ostream& s){ s<<"ka"; }

#endif
}


