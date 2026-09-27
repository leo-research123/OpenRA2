#pragma once

class AbstractTypeClass;
struct ColorStruct;
struct SHPStruct;
class CCINIClass;
class ColorScheme;
template<class T> class DynamicVectorClass;
enum class AbstractType : unsigned int;

namespace game {
// Dependencies owned by the existing game type/audio registries. This stores
// no rule/type objects. Returned objects are borrowed until their registry is
// cleared; names and callback arguments are borrowed for the current call.
struct RulesRuntimeServices {
    void* context = nullptr;
    // Execute the original class's FindOrAllocate contract, including NONE and
    // <none>, registration and allocation failure. A null output can be a
    // successful resolution. False means the dependency could not execute.
    bool (*resolve)(void*, AbstractType, const char*, AbstractTypeClass*&) = nullptr;
    bool (*sound_index)(void*, const char*, int&) = nullptr;
    bool (*type_count)(void*, AbstractType, int&) = nullptr;
    bool (*type_at)(void*, AbstractType, int, AbstractTypeClass*&) = nullptr;
    // Follows the original class's index lookup, which can allocate: notably
    // ParticleSystemType::From_Name treats only empty / <none> as absent.
    bool (*find_index)(void*, AbstractType, const char*, int&) = nullptr;
    bool (*finalize_weapon)(void*, AbstractTypeClass*) = nullptr;
    bool (*register_color)(void*, const char*, const ColorStruct&) = nullptr;
    bool (*create_color_scheme)(void*, const char*, const ColorStruct&, int shades) = nullptr;
    bool (*command_position)(void*, int command, int position) = nullptr;
    bool (*command_count)(void*, int count) = nullptr;
    // Borrowed current sentinel, normally -1 in the fixed target.
    const int* no_command = nullptr;
    bool (*shape)(void*, const char* name, SHPStruct*&) = nullptr;
    // Sound-list loading uses VocClass's allocating lookup, unlike scalar
    // sound_index. A non-null original record can still have index -1.
    bool (*sound_list_entry)(void*, const char*, bool& exists, int& index) = nullptr;
    DynamicVectorClass<ColorScheme*>* color_schemes = nullptr;
    // Destruction uses the owning original class and removes its registry
    // entry. Only objects already owned by that registry may be passed here.
    bool (*destroy_color_scheme)(void*, ColorScheme*) = nullptr;
    bool (*clear_color_tables)(void*) = nullptr;
    bool (*destroy_first_type)(void*, AbstractType) = nullptr;
    const int* session_mode = nullptr;
    bool (*mode_ini)(void*, CCINIClass*&) = nullptr;
    bool (*read_tiberiums)(void*, CCINIClass*) = nullptr;
    // Configuration-only audio references. The owner retains the complete
    // source INI; unresolved names are reported, never turned into fake IDs.
    // No sound device or sample is opened. Default keeps original resolution.
    bool defer_sound_references = false;
    bool (*retain_sound_reference)(void*, const CCINIClass*, const char* section,
        const char* key) noexcept = nullptr;
};

// Native registry services for the locally constructed types only. Missing
// type/audio/color/world capabilities return false; they never invoke EXE
// constructors or create proxy objects. Returned service table is immutable.
const RulesRuntimeServices& native_rules_runtime() noexcept;

// Synchronous borrowed dependency scope. Nested calls restore the previous
// scope even if operation throws. False rejects a null operation. Exceptions
// from an unavailable dependency/operation propagate; they are not success.
// Hosts serialize modifications of the actual game registries.
bool with_rules_runtime(const RulesRuntimeServices& services,
    void (*operation)(void*), void* context);
}
