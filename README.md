# Scientific Calculator for Arduino

I made this a while ago, and might touch it up eventually  
It compiles on wowki, and can be used [here](https://wokwi.com/projects/445937649533105153)

## Usage

Operations are entered using two-key combinations on the keypad  
The first key (`A`, `B`, `C`, or `D`) selects a function group, and the second key chooses the specific operator

| Combo | Operation | Symbol |
|:-|:-|:-|
| `A + A` | Addition | `+` |
| `A + B` | Subtraction | `-` |
| `A + C` | Multiplication | `*` |
| `A + D` | Division | `/` |
| `A + .` | Left Parenthesis | `(` |
| `A + #` | Right Parenthesis | `)` |
| `A + 2` | **e** | `e` |
| `A + 3` | **π** | `π` |
| `B + A` | Exponentiation | `^` |
| `B + B` | Square Root | `√` |
| `B + C` | Log base 10 | `log` |
| `B + D` | Natural Log | `ln` |
| `C + A` | Sine | `sin` |
| `C + B` | Cosine | `cos` |
| `C + C` | Tangent | `tan` |
| `D + A` | Factorial | `!` |
| `%` | Delete | – |
| `#` | Enter (Evaluate) | – |

Numbers and decimals (`0–9` and `.`) are entered directly  
Implicit multiplication is supported!

Calculate expression with `#`, and `#` again to reset

## Features
- 4x4 keypad input with combo keys for operators to build functions  
- LCD display output for live input and results  
- Supports:
  - `+`, `-`, `*`, `/`, `^`
  - `sqrt`, `log`, `ln`
  - `sin`, `cos`, `tan`
  - constants: `π`, `e`
  - factorial `!`
  - implicit multiplication

## Hardware

- **LCD:** 16x2 display connected to analog pins `A0–A5`
- **Keypad:** 4x4 matrix connected to digital pins `2–9`