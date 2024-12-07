Below is a revised specification, reframed as a flexible specification framework for defining and verifying extensions according to safety, modifiability, and isolation requirements. This version incorporates the concepts of constraints, capabilities, helpers, and extension entries into a more unified model, merging the previous notion of roles and hooks into a single extension entry definition. It also clarifies the two classes of constraints and highlights the verification aspect as central to the Extension Interface Model (EIM).

---

# Extension Interface Model (EIM) Specification Framework

## 1. Introduction

This specification framework describes a model (EIM) that enables a host environment to define, load, and verify dynamically supplied extensions. The goal is to ensure that each extension operates within rigorously defined boundaries, adheres to declared safety constraints, and invokes only authorized host-provided functions (helpers). By enforcing constraints on types, capabilities, and allowed behaviors, the EIM fosters a least-privilege design where extensions can be safely and dynamically integrated.

## 2. Key Concepts

The EIM is built around the following core concepts:

- **Types**: Associate base data types with constraints, ensuring values meet specific conditions (e.g., non-null, positive).
- **Extension-Exported Entries (Extension Functions)**: The extension’s callable interfaces that the host can invoke. These define the extension’s behavior, preconditions, and postconditions.
- **Host-Provided Functions (Helpers)**: Functions defined by the host that an extension may call if permitted. They represent controlled gateways to system resources and functionality.
- **Capabilities**: Structured permission bundles, each grouping a set of allowed helpers and imposing associated constraints.
- **Constraints**: Logical conditions that enforce safety, correctness, and resource usage limits at every level of the model.

The EIM is a specification framework: it allows system designers to define their own set of types, constraints, capabilities, and extension entries based on their particular safety, modifiability, and isolation trade-offs.

## 3. Normative References

The terms "MUST", "MUST NOT", "SHOULD", and "SHOULD NOT" used in this specification are to be interpreted as defined in [RFC 2119].

Unless explicitly stated otherwise, all statements are normative requirements.

## 4. Terminology

- **Extension**: Dynamically loaded code that conforms to the EIM specification and can be invoked by the host through defined extension-exported entries.
- **Host Environment**: The runtime or system that loads the extension, provides host helpers, and verifies compliance.
- **Verifier**: A component that checks the extension’s compliance with all specified constraints before allowing execution.

## 5. Types

### Definition

A **Type** `τ` is defined as `(BaseType, Constraints(τ))`, where:

- **BaseType**: A primitive or composite data type (e.g., `int`, `size_t`, `char*`, or a struct).
- **Constraints(τ)**: A finite set of constraints that must hold for any value of that type (e.g., `value ≥ 0`, `non_null`).

### Requirements

- Type constraints MUST be verifiable by static or symbolic methods before runtime, ensuring safe usage patterns.
- All parameters and return values in extension entries and helpers MUST specify their type and must adhere to that type’s constraints.

### Example

```yaml
types:
  - name: "int_positive"
    base: "int"
    constraints:
      - "value ≥ 0"

  - name: "const_char_ptr"
    base: "char*"
    constraints:
      - "non_null"
      - "null_terminated"
```

## 6. Constraints

An EIM uses constraints as its primary mechanism to enforce extension safety and correctness. Constraints can appear at multiple levels: types, functions, capabilities, and extension entries.

### Two Classes of Constraints

1. **Relational Constraints**: These express logical relationships between variables, constants, or special values. For example:
   - `value ≥ 0`
   - `memory_usage ≤ 65536`
   - `time < inf` (resource limit)

   These constraints must be statically verifiable or safely checkable at runtime.

2. **Tag-Based Constraints**: These declare properties of a program or object. For example:
   - `read_only` (indicating that a capability allows only non-mutating operations)
   - `allocator`/`deallocator` tags (indicating a function that allocates or frees resources)
   - `side_effects = false` (indicating no side effects are allowed from this entry or helper)

### Requirements

- Constraints MUST be decidable.
- Violating any constraint (type-level, function-level, capability-level, or extension-entry-level) MUST cause verification to fail.
- Constraints can encode both functional correctness conditions (e.g., ensuring a return value is non-negative) and resource constraints (e.g., limiting memory usage or prohibiting certain side effects).

## 7. Host-Provided Functions (Helpers)

**Helpers** are host functions accessible to the extension if allowed by the extension’s configuration. They represent controlled operations, such as reading a file, sending a packet, or allocating memory.

### Definition

A host helper `h` is defined as:

```makefile
h = (Name_h, (τ1, τ2, ..., τn) -> τ_out, Helper_Constraints)
```

- `(τ1, τ2, ..., τn)`: Parameter types, each with type-level constraints.
- `τ_out`: Return type with type-level constraints.
- `Helper_Constraints`: Additional pre- and postcondition constraints (e.g., `return ≥ -1`, `side_effects = network`).

### Requirements

- Each helper MUST specify its signature and constraints.
- If an extension attempts to call a helper with arguments or conditions violating these constraints, the verifier MUST prevent execution.

### Example

```yaml
host_functions:
  - name: "host_read_file"
    signature:
      params:
        - { type: "int_positive", name: "fd" }
      return_type: "int"
    function_constraints:
      - "return ≥ -1"
```

If `fd` is negative, calling `host_read_file` violates the `int_positive` constraint, causing verification to fail.

## 8. Capabilities

**Capabilities** define sets of helpers and associated constraints, functioning as permission bundles. By granting an extension certain capabilities, the host controls which helpers it can invoke.

### Definition

```makefile
Capability = (Name_c, H_c, Cap_Constraints)
```

- `Name_c`: The capability’s name.
- `H_c`: The set of host-provided functions included in this capability.
- `Cap_Constraints`: Constraints applied to this capability. For example, `read_only = true` would prohibit helpers that write data.

### Requirements

- The extension can only invoke helpers from capabilities it is allowed to use.
- If a capability requires `read_only = true`, it cannot include helpers that perform write operations.
- If conflicting constraints arise (e.g., adding a write helper to a `read_only` capability), verification MUST fail.

### Example

```yaml
capabilities:
  - name: "FileRead"
    apis: ["host_read_file"]
    constraints:
      - "read_only = true"
```

`FileRead` capability restricts the extension to read-only file operations.

## 9. Extension-Exported Entries (Extension Functions)

Extension-exported entries define the interface points (extension functions) that the host can call into the extension. They represent the extension’s outward-facing functions or hooks. This model merges the concept of roles and hooks into a single unit: each extension entry comes with its own constraints and allowable capabilities.

### Definition

An **Extension-Exported Entry** `e` is defined as:

```makefile
e = (Name_e, (τ1, τ2, ..., τm) -> τ_out, E_Constraints, Allowed_Capabilities)
```

Where:

- `Name_e`: The entry function’s name.
- `(τ1, τ2, ..., τm) -> τ_out`: Parameter and return types, each with type-level constraints.
- `E_Constraints`: Constraints specific to this entry (e.g., `memory_usage ≤ 65536`, `side_effects = false`).
- `Allowed_Capabilities`: A set of capabilities that this entry can use. By referencing capabilities by name, we allow the extension to invoke the helpers in those capabilities.

### Requirements

- Each extension-exported entry MUST declare its parameter and return types, constraints, and allowed capabilities.
- If the extension’s logic or usage of helpers conflicts with the entry’s constraints, the verifier MUST fail verification.
- By specifying allowed capabilities at the extension entry level, we control what host functionality each entry can leverage.

### Example

Consider capabilities:

```yaml
capabilities:
  - name: "NetAccess"
    apis: ["host_send_packet"]
    constraints:
      - "side_effects = network"

  - name: "FileRead"
    apis: ["host_read_file"]
    constraints:
      - "read_only = true"
```

An extension entry might look like:

```yaml
extension_entries:
  - name: "process_data"
    signature:
      params:
        - { type: "int_positive", name: "count" }
      return_type: "int_positive"
    constraints:
      - "return ≥ 0"
      - "memory_usage ≤ 65536"
      - "side_effects = false"
    allowed_capabilities:
      - "FileRead"    # allows host_read_file
      # "NetAccess" not listed, so not allowed
```

Here, `process_data` can read files (due to `FileRead` capability) but not send network packets (`NetAccess` is not included).

## 10. Verification Process

Before loading an extension, the host’s verifier performs these checks:

1. **Type Checking**: Ensures that all parameters and return values in extension entries and helpers adhere to their declared type constraints.
2. **Constraint Compliance**: Verifies that all relational and tag-based constraints at every level (type, helper, capability, extension entry) are satisfied.
3. **Capability Matching**: Confirms that the extension invokes only helpers from the allowed capabilities specified by its extension entries.
4. **Safety and Resource Checks**: Ensures that resource constraints (e.g., `time < inf`, `memory_usage ≤ limit`) and safety conditions (`side_effects = false`) are met.
5. **Error Handling**: If any requirement fails, verification MUST fail, preventing extension loading or execution.

If verification succeeds, the extension can be safely loaded and run under the specified constraints.

## 11. Non-Normative Notes

The EIM framework is not object-oriented, but one can think of capabilities as analogous to "permission sets" and extension entries as "methods" with explicit pre/post conditions. Types and constraints function as a contract system, ensuring correct and safe use of host interfaces. Although the analogy is informal, it may help implementers conceptualize the model.

---

## Conclusion

The Extension Interface Model (EIM) provides a formal, flexible framework for specifying, verifying, and running extensions safely. By combining typed interfaces, host-provided helpers, capabilities, and a rich constraint system, EIM enables a principle-of-least-privilege approach. This ensures that dynamically loaded extensions operate correctly within predefined boundaries, supporting a spectrum of safety, modifiability, and isolation trade-offs.
