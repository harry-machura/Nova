# Nova – Eine kompakte Programmiersprache mit Compiler & VM

[![Build](https://img.shields.io/github/actions/workflow/status/<dein-user>/nova/ci.yml?branch=main&logo=github)](https://github.com/<dein-user>/nova/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-linux%20%7C%20macos%20%7C%20windows-blue)](#installation--build)
[![Release](https://img.shields.io/github/v/release/<dein-user>/nova)](https://github.com/<dein-user>/nova/releases)

> **Nova** ist eine kleine, produktionsreife Sprache mit **C-ähnlicher Syntax**, einem **Bytecode-Compiler (`novac`)** und einer **portablen virtuellen Maschine (`novavm`)**.  
> Der Fokus liegt auf **Lesbarkeit**, **vorhersagbarer Performance**, **kleinem Runtime-Footprint** und **einfacher Einbettung** (C-ABI/FFI).

---

## Features

- **Einfach, lesbar, schnell** – Syntax inspiriert von C/Go/Rust  
- **Kompilierung zu Bytecode** – `novac` erzeugt portable `.nvc` Dateien  
- **Leichte VM** – `novavm` führt Bytecode deterministisch aus  
- **Statische Typen + Inferenz**  
- **Standardbibliothek** – Strings, Collections, IO, Math, Zeit  
- **FFI zu C** – Native Funktionen einfach binden  
- **Saubere Fehlerbehandlung** – `result<T,E>`, `try`, `defer`  

---

## Installation & Build

### Voraussetzungen
- CMake ≥ 3.20  
- C/C++-Compiler (Clang, GCC oder MSVC)  
- (optional) Ninja für schnelleren Build  

### Build unter Linux/macOS
```bash
git clone https://github.com/<dein-user>/nova.git
cd nova
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Build unter Windows (Visual Studio 2022)
```powershell
git clone https://github.com/<dein-user>/nova.git
cd nova
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

**Artefakte:**
- `build/novac` – Nova Compiler  
- `build/novavm` – Nova VM  

---

## Schnellstart

```bash
# Hello World in Nova
cat > hello.nova <<'NV'
module main;

func main() -> i32 {
    println("Hello, Nova!");
    return 0;
}
NV

# Kompilieren
./build/novac hello.nova -o hello.nvc

# Ausführen
./build/novavm hello.nvc
```

Ausgabe:
```
Hello, Nova!
```

---

## Beispiele

Im Repo findest du weitere Programme unter [`examples/`](examples/):

- [`examples/hello.nova`](examples/hello.nova) – klassisches Hello World  
- [`examples/math.nova`](examples/math.nova) – Funktionen, Structs  
- [`examples/async.nova`](examples/async.nova) – Nebenläufigkeit mit `spawn` & `chan`  

---

## Projektstruktur

```
nova/
├─ src/             # Compiler + VM Quellcode
│  ├─ front/        # Lexer, Parser, Typchecker
│  ├─ middle/       # IR, Optimierungen
│  ├─ back/         # Bytecode-Emitter
│  └─ vm/           # Virtuelle Maschine
├─ std/             # Standardbibliothek
├─ examples/        # Beispielprogramme
├─ tests/           # Unit- und Integrationstests
└─ README.md        # Diese Datei
```

---

## Dokumentation

- [Syntax-Referenz](docs/syntax.md) – vollständige Grammatik (EBNF)  
- [Standardbibliothek](docs/stdlib.md) – Beschreibung aller Module (`io`, `math`, `sys`, …)  
- [FFI](docs/ffi.md) – C-Funktionen aus Nova nutzen  
- [Embedding](docs/embedding.md) – Nova-VM in C/C++ Projekten einbetten  

---

## Contributing

Pull Requests sind willkommen! Bitte beachte:  

- Code mit `clang-format` formatieren  
- Unit Tests hinzufügen (`ctest`)  
- Commit-Style: [Conventional Commits](https://www.conventionalcommits.org)  

---

## Lizenz

MIT – siehe [LICENSE](LICENSE).
