
# Nova Syntax (MVP)

> Diese Datei beschreibt die aktuell implementierte Minimal-Syntax des Compilers `novac` und der VM `novavm` – inklusive **Strings**.

## Programmaufbau
Ein Programm ist eine Sequenz von Statements. Es gibt keine Semikolons; **Zeilenumbrüche** und **Blockklammern `{}`** trennen Statements.

## Statements
- `let name = expr` – deklariert eine neue Variable (globaler Slot)
- `name = expr` – weist einer existierenden Variable zu
- `print(expr)` – gibt `expr` ohne Zeilenumbruch aus (int oder string)
- `println(expr)` – wie `print`, aber mit Zeilenumbruch
- `if (expr) { block } [else { block }]`
- `while (expr) { block }`
- Block: `{ ... }` (keine neue Scope-Tabelle, Slots sind global)

## Ausdrücke
- Literale: `123`, `"text"`, `true`/`false` (Booleans entstehen aus Vergleichen; als int `0/1`)
- Variablen: `name`
- Klammerung: `(expr)`

### Operator-Präzedenz (hoch → niedrig)
1. unär: `-x`, `!x`
2. `* / %`
3. `+ -`
4. Vergleiche: `== != < <= > >=`
5. Logik: `&& ||` (ohne Kurzschlussauswertung im MVP)

Alle Operatoren arbeiten mit **Integern (`i32`)**. Strings werden aktuell **nur** als Operanden von `print/println` unterstützt.

## Beispiele

```nova
let greeting = "Hello, Nova!"
println(greeting)

let a = 2
let b = 40
println(a + b)          // 42

let i = 0
while (i < 3) {
  println("i=")
  println(i)
  i = i + 1
}

if (a + b == 42) {
  println("ok")
} else {
  println("nope")
}
```

## Bytecode-Format
- Magic: `"NOVABC01"`
- String-Pool:
  - `u32 n` Anzahl Strings
  - Wiederholt: `u32 len` + `len` Bytes UTF-8
- Code: `u32 code_size` + Bytecode

## Hinweise
- Variablen-Slots: max. 256. Keine Shadowing/Scopes im MVP.
- Division/Modulo durch 0 → Laufzeitfehler.
- `&&`/`||` evaluieren beide Seiten (kein Kurzschluss im MVP).
