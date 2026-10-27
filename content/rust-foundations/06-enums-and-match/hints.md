Start each function with `match s { ... }` and one arm per variant, binding the data: `Shape::Circle(r) => ...`.
===
Circle area is `PI * r * r`, circumference `2.0 * PI * r`. Rectangle area is `w * h`, perimeter `2.0 * (w + h)`. Triangle perimeter is `a + b + c`.
===
Triangle area: `let p = (a + b + c) / 2.0; (p * (p - a) * (p - b) * (p - c)).sqrt()`. Inside a match arm use braces for several lines: `Shape::Triangle(a, b, c) => { ... }`. If the compiler complains about `&f64`, dereference with `*a`.
