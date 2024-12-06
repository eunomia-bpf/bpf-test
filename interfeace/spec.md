# Extension Model Specification

## Overview

The Extension Permission and Control Model (EPCM) provides a structured, formally verifiable framework to control the behavior of dynamically loaded extensions within a host environment. EPCM defines *roles*, *capabilities*, and *constraints*, and uses *typed interfaces* to ensure that every interaction between extensions and the host environment adheres to specified conditions. EPCM supports inheritance and composition to allow scalable, modular configurations, and is designed for formal verification at load time or during pre-deployment analysis.

Key ideas:

1. **Roles**: Assign permission bundles (capabilities and constraints) to extensions.
2. **Capabilities**: Define sets of allowed host APIs with well-defined type signatures and associated constraints.
3. **Constraints**: Impose logical conditions on types, host functions, resources, operations, and roles themselves. Constraints unify what were previously resource/operational "attributes" and logical conditions. All permissible actions and states are governed by constraints.
4. **Typed Interfaces**: Ensure that every host and extension function adheres to declared type-based constraints, enabling static or symbolic verification.
5. **Inheritance and Composition**: Support scalable, modular configuration of permissions and constraints by composing capabilities and inheriting roles.
6. **Formal Verification**: The model and its configurations are designed for verification by a verifier that checks correctness, consistency, and adherence to constraints at load time or pre-deployment.

---

## Formal Language and Definitions

### Basic Sets and Domains

- Let `H` be a finite set of **Host APIs** (host-provided functions).
- Let `E` be a finite set of **Extension Exported Functions** (extension-provided entry points).
- Let `T` be a set of **Types**, each associated with type-level constraints.
- Let `C` be a set of **Capabilities**.
- Let `R` be a set of **Roles**.
- All conditions, operational policies, resource limits, and logical requirements are expressed as **Constraints**.

We use a specification language (e.g., YAML/JSON or a DSL) to declare types, capabilities, roles, and constraints. The semantics remain consistent regardless of the chosen format.

### Types and Constraints

**Definition (Type):**  
A type `τ ∈ T` is defined as `(BaseType, Constraints(τ))`, where:
- `BaseType` is a primitive or composite data type (e.g., `int`, `size_t`, `char*`, or a struct).
- `Constraints(τ)` is a finite set of logical predicates that must hold for any value of that type. Examples:
  - Non-null pointer: `value ≠ NULL`
  - Bounded integer: `0 ≤ value ≤ 100`
  - Null-terminated string: `∀ i, value[i] ≠ '\0'` until a null terminator is found

**Type System Requirements:**
- For each `τ ∈ T`, `Constraints(τ)` must be decidable, enabling static or symbolic verification.
- The verifier checks, for every function call, that arguments and return values meet these type constraints.

**Example Type Definitions:**
```yaml
types:
  - name: "const_char_ptr"
    base: "char*"
    constraints: ["non_null", "null_terminated"]

  - name: "int_positive"
    base: "int"
    constraints: ["value ≥ 0"]
```

### Host APIs

**Definition (Host Function):**  
Each host function `h ∈ H` is defined as:
```
h = (Name_h, (τ1, τ2, ..., τn) -> τ_out, F_Constraints)
```
- `Name_h`: A unique identifier (string).
- `(τ1, τ2, ..., τn)`: Parameter types subject to their type constraints.
- `τ_out`: Return type subject to its type constraints.
- `F_Constraints`: Additional logical conditions on usage. For example, if `τ_out` is an integer representing a file descriptor, we might have:
  - "If return < 0, an error occurred."
  - "filename must be non_null and null_terminated."
  - If the host function has side effects, such as creating files or sending network packets, it may define constraints like `may_have_side_effect = true`. This is also a constraint, stating that this function’s invocation changes global state, and thus roles granting access to it must permit side effects.

**Verifier Checks for Host APIs:**
- Argument and return values must satisfy type constraints.
- Logical constraints (e.g., error conditions, side-effect permissions) must hold for every call.
- The verifier may track resource allocations (e.g., memory) and ensure no resource leaks or violations of resource-related constraints occur.

**Example in C:**
```c
int host_open_file(const char *filename, const char *mode);
void* host_malloc(size_t size);
void host_free(void* ptr);
```

For `host_open_file`, constraints might state:
- `filename.non_null`
- `mode.non_null`
- `return ≥ -1`
- If `return < 0`, an error is signaled.

### Extension Exported Functions

Each extension-provided entry point `e ∈ E` is similarly defined:
```
e = (Name_e, (τ1, τ2, ..., τm) -> τ_out, E_Constraints)
```
- The host calls these functions, and they must also adhere to all declared constraints.
- For example, if an extension entry expects `int_positive`, it must not receive negative values.

**Example:**
```yaml
extension_exports:
  - name: "process_data"
    signature:
      params:
        - {type: "int_positive", name: "count"}
      return_type: "const_char_ptr"
    constraints:
      - "If return ≠ NULL, it must be null_terminated"
```

### Capabilities

**Definition (Capability):**
```
c = (Name_c, H_c, Cap_Constraints)
```
- `Name_c`: Unique capability name.
- `H_c ⊆ H`: A subset of host APIs that the extension can invoke if granted this capability.
- `Cap_Constraints`: Additional constraints applying to the use of these APIs as a group. For instance, a capability might state that certain APIs must not be called more than a certain number of times, or that all file descriptors returned must be validated before use.

**Capability Composition:**
If we have `c1 = (Name_c1, H_c1, CapCon1)` and `c2 = (Name_c2, H_c2, CapCon2)`, a composed capability:
```
c_comp = (Name_comp, H_c1 ∪ H_c2, CapCon_comp)
```
`CapCon_comp` merges constraints from `CapCon1` and `CapCon2`. If contradictions occur (e.g., one capability requires `read_only = true` and the other requires `read_only = false`), composition fails unless explicitly resolved.

### Constraints (formerly Attributes)

All operational and resource limitations that were previously termed "attributes" are now expressed as constraints. Instead of saying `max_memory: 1048576`, we define a constraint: `memory_usage ≤ 1048576`.

Common constraints include:
- Resource constraints (e.g., `memory_usage ≤ 1048576`).
- Operational constraints (e.g., `no_network_access` means no network-related APIs are allowed).
- Side-effect constraints (e.g., `no_memory_writes` means the extension cannot invoke APIs that modify host memory).
- Instruction count constraints (`instruction_count ≤ 1000000`).

These constraints are logical conditions integrated into the role or capability definitions. The verifier ensures no violation of these conditions occurs.

**Example:**
```yaml
capabilities:
  - name: "FileRead"
    apis: ["host_open_file", "host_read_file"]
    constraints:
      - "All returned file descriptors must be checked before use"
      - "read_only = true"

roles:
  - name: "BasicFileRole"
    capabilities: ["FileRead"]
    # Instead of attributes, we now write constraints directly:
    constraints:
      - "memory_usage ≤ 1048576"
      - "instruction_count ≤ 1000000"
      - "no_memory_writes"
      - "no_network_access"
```

### Roles

**Definition (Role):**
```
R = (Name_R, Capabilities_R, Constraints_R, Parents_R)
```
- `Name_R`: Unique role name.
- `Capabilities_R ⊆ C`: The capabilities included in this role.
- `Constraints_R`: A set of constraints (including what were previously resource and operational limits).
- `Parents_R ⊆ R`: Zero or more parent roles from which this role inherits capabilities and constraints.

**Inheritance Rules:**
- Capabilities: Inherited from parents plus those defined in the role.
- Constraints: Inherited and combined. If a parent sets `no_memory_writes` and the child sets `allow_memory_writes`, the child must explicitly resolve this contradiction or the role is invalid.
- All constraints must remain satisfiable. Contradictory or unsatisfiable constraints cause verification to fail.

**Effective Role Permissions:**
The verifier merges constraints and capabilities from all ancestors to produce a final, conflict-free set of constraints and allowed APIs.

### Assigning Roles to Extensions

When loading an extension `X`, the host assigns one or more roles. The intersection of all constraints from these roles governs what `X` can do. If any contradictions arise, verification fails and `X` is not loaded.

---

## Formal Verification Approach

A verifier runs at deployment or load time. It checks:

1. **Type Consistency**:  
   Ensures all arguments and return values satisfy the type-level constraints. For example, if `int_positive` is required and negative values are possible, verification fails.

2. **Constraint Compliance**:  
   The verifier uses static analysis, symbolic execution, or other logic to ensure that no action taken by the extension violates any constraints. For example:
   - `memory_usage ≤ 1048576` means it must not allocate beyond 1MB.
   - `no_memory_writes` means no API calls that write memory are used.
   - `read_only = true` within a capability means no write-oriented API is called.

3. **Capability and Role Composition**:  
   Checks if all composed capabilities and inherited roles produce a logically consistent set of constraints. If contradictions are found, verification fails.

4. **Non-functional Properties (Optional)**:  
   The verifier may also check that no null dereferences or buffer overflows occur if specified by constraints (e.g., `non_null` pointers, bounded buffers).

**Verification Algorithmic Aspects:**
- **Type Checking**: Confirm type constraints.
- **Symbolic Execution**: Explore code paths to ensure no constraint violations.
- **Policy-Driven Resolution**: If multiple parents define conflicting constraints, a predefined policy might dictate which is chosen or require the child to specify which constraint takes precedence.

If verification succeeds, the extension is considered safe and loaded; if not, loading is aborted.

---

## Additional Examples

### Example 1: Network-Restricted Role

```yaml
types:
  - name: "char_buf"
    base: "char*"
    constraints: ["non_null", "null_terminated"]

host_functions:
  - name: "host_send_packet"
    signature:
      params:
        - {type: "char_buf", name: "data"}
        - {type: "int", name: "len"}
      return_type: "int"
    function_constraints:
      - "data.non_null"
      - "len ≥ 0"
      - "return ≥ -1"
      - "If return < 0, error occurred."
      - "side_effects = network"

capabilities:
  - name: "NetworkSend"
    apis: ["host_send_packet"]
    constraints:
      - "side_effects = network"

roles:
  - name: "NoNetworkRole"
    constraints:
      - "no_network_access"

  - name: "NetRole"
    capabilities: ["NetworkSend"]
    # Must allow network if we want to use network-related APIs
    constraints:
      - "network_access = true"

# If you assign an extension both "NoNetworkRole" and "NetRole",
# their constraints conflict (one says no network, the other says network access required).
# Verification fails unless resolved by explicit policy.
```

### Example 2: Memory-Bounded Role with Composed Capabilities

```yaml
host_functions:
  - name: "host_malloc"
    signature:
      params:
        - {type: "size_t", name: "size"}
      return_type: "void_ptr"
    function_constraints:
      - "size ≥ 0"
      - "return ≠ NULL if size > 0 else return may be NULL"
      - "memory_allocated(size)"

  - name: "host_free"
    signature:
      params:
        - {type: "void_ptr", name: "ptr"}
      return_type: "void"
    function_constraints:
      - "ptr.non_null"
      - "memory_released(ptr)"

capabilities:
  - name: "MemoryAlloc"
    apis: ["host_malloc", "host_free"]
    constraints:
      - "memory_management_allowed"

roles:
  - name: "MemLimitedRole"
    capabilities: ["MemoryAlloc"]
    constraints:
      - "memory_usage ≤ 524288"  # Limit memory to 512KB

# The verifier ensures that the extension doesn't exceed 512KB total memory usage.
```


