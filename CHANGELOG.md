# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.0.2] - 2019-07-06
### Added
- Static evaluation on `if` and `for` statements.
- Hashing to external verilog (#2).
- Skip hashing if the generator is unique (#3).
- Collapse else if into actual else if in verilog (#4).
- Bypass python's lack of switch statement (#5).
- Add a way to specify verilog stubs (#6).
- Add cache to instantiate generators (#11).
- Transform == into eq call (#14).
- Add switch code gen (#15).
- Add pass through module detection and removal pass (#17).
- Add pass to remove chains of fan-out 1 wires (#18).
- Add a way to manage passes (#19).
- Add full trace of statements for debugging (#24).

### Changed
- Verilog code generation is refactored to be more consistent with each other (#1)
- Refactor how instance name is done (#7).
- Unify comb and seq add interface in Python (#16).
- Refactor ast visit (#21).
- Allow arbitrary for loop (#22).
- Refactor how sources and sinks are handled in slices (#25).

### Fixed
- Hashing not picking up the variable width (#8).
- Detect the output being used as a register (#10).
- Extra "end" for else if statement (#12).

## [0.0.1] - 2019-06-27
### Added
- Initial release of kratos.
