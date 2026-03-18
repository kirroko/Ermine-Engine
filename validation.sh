#!/bin/bash

# ==============================================================================
# Ermine Engine CI - Asset, Data, and Configuration Validation Script
# ==============================================================================
# This script validates:
# - Asset structure and metadata integrity
# - Configuration files (JSON, config)
# - Resource database consistency
# - Shader validity
# - Material asset references
# - Scene file integrity
# ==============================================================================

# set -e # Exit on any error
set -uo pipefail # Catch errors in pipelines

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd -- "$(dirname -- "$(realpath -- "${BASH_SOURCE[0]}")")" && pwd -P)"
PROJECT_ROOT="$SCRIPT_DIR"
RESOURCES_DIR="$PROJECT_ROOT/Resources"
RESOURCES_VALIDATION_DIR="$(cd -- "$PROJECT_ROOT/../../../Resources" && pwd -P)"

# Compare source assets against build assets
SOURCE_RESOURCES_DIR="${SOURCE_RESOURCES_DIR:-$RESOURCES_VALIDATION_DIR}"
BUILD_RESOURCES_DIR="${BUILD_RESOURCES_DIR:-$RESOURCES_DIR}"

MATERIALS_VALIDATION_DIR="${RESOURCES_VALIDATION_DIR}/Materials"
SHADERS_VALIDATION_DIR="${RESOURCES_VALIDATION_DIR}/Shaders"
SCENES_VALIDATION_DIR="${RESOURCES_VALIDATION_DIR}/Scenes"
TEXTURES_VALIDATION_DIR="${RESOURCES_VALIDATION_DIR}/Textures"

MATERIALS_DIR="${RESOURCES_DIR}/Materials"
SHADERS_DIR="${RESOURCES_DIR}/Shaders"
SCENES_DIR="${RESOURCES_DIR}/Scenes"
TEXTURES_DIR="${RESOURCES_DIR}/Textures"
CONFIG_DIR="${PROJECT_ROOT}/Config"
ERMINE_GAME_DB="${PROJECT_ROOT}/Ermine-Game.lion_rcdbase"

# Statistics
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0
WARNINGS=0

VERBOSE=0

print_usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message and exit"
    echo "  -v, --verbose       Enable verbose output"
    echo ""
    echo "Environment Variables:"
    echo "  PROJECT_ROOT        Root directory of the project (default: current directory)"
    echo "  SOURCE_RESOURCES_DIR Directory containing source assets for validation (default: ../../../Resources)"
    echo "  BUILD_RESOURCES_DIR Directory containing built assets for validation (default: ./Resources)"
}

while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)
            print_usage
            exit 0
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            print_usage
            exit 2
            ;;
    esac
    shift
done

# ==============================================================================
# Utility Functions
# ==============================================================================

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    if [ "$VERBOSE" -eq 1 ]; then
        echo -e "${GREEN}[PASS]${NC} $1"
    fi
    ((PASSED_CHECKS++))
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAILED_CHECKS++))
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
    ((WARNINGS++))
}

((TOTAL_CHECKS++))

# ==============================================================================
# Asset Structure Validation
# ==============================================================================

validate_build_file_presence() {
    log_info "Validating build files exist for all sources assets..."
    log_info "Source: $SOURCE_RESOURCES_DIR"
    log_info "Build:  $BUILD_RESOURCES_DIR"

    if [ ! -d "$SOURCE_RESOURCES_DIR" ]; then
        ((TOTAL_CHECKS++))
        log_error "Source resources directory not found: $SOURCE_RESOURCES_DIR"
        return 1
    fi

    if [ ! -d "$BUILD_RESOURCES_DIR" ]; then
        ((TOTAL_CHECKS++))
        log_error "Build resources directory not found: $BUILD_RESOURCES_DIR"
        return 1
    fi

    local source_count=0
    local missing_count=0
    local orphaned_in_build=0

    log_info "Checking for missing files in build directory..."
    while IFS= read -r -d '' src_file; do
        ((TOTAL_CHECKS++))
        ((source_count++))

        local rel_path="${src_file#$SOURCE_RESOURCES_DIR/}"
        local build_file="$BUILD_RESOURCES_DIR/$rel_path"

        if [ -f "$build_file" ]; then
            src_size=$(stat -c%s "$src_file")
            build_size=$(stat -c%s "$build_file")
            if [ "$src_size" -eq "$build_size" ]; then
                log_success "Build file exists: $rel_path"
            else
                log_warning "Build file size mismatch for $rel_path (Src: $src_size bytes, Build: $build_size bytes)"
            fi
        else
            log_error "Missing in build: $rel_path"
            ((missing_count++))
        fi
    done < <(find "$SOURCE_RESOURCES_DIR" -type f -print0)

    log_info "Checking for orphaned files in build directory..."
    while IFS= read -r -d '' bld_file; do
        local rel_path="${bld_file#$BUILD_RESOURCES_DIR/}"
        local source_file="$SOURCE_RESOURCES_DIR/$rel_path"

        if [ ! -f "$source_file" ]; then
            ((TOTAL_CHECKS++))
            log_warning "Orphaned build file (not in source): $rel_path"
            ((orphaned_in_build++))
        fi
    done < <(find "$BUILD_RESOURCES_DIR" -type f -print0)

    if [ "$source_count" -eq 0 ]; then
        ((TOTAL_CHECKS++))
        log_warning "No source files found to validate in $SOURCE_RESOURCES_DIR"
    fi

    if [ "$missing_count" -gt 0 ] || [ "$orphaned_in_build" -gt 0 ]; then
        return 1
    fi

    return 0
}

validate_directory_structure() {
    log_info "Validating directory structure..."
    
    local required_dirs=(
        "$RESOURCES_DIR"
        "$MATERIALS_DIR"
        "$SHADERS_DIR"
        "$TEXTURES_DIR"
    )
    
    for dir in "${required_dirs[@]}"; do
        ((TOTAL_CHECKS++))
        if [ -d "$dir" ]; then
            log_success "Directory exists: $dir"
        else
            log_error "Required directory missing: $dir"
        fi
    done
}

# ==============================================================================
# Configuration File Validation
# ==============================================================================

validate_config_files() {
    log_info "Validating configuration files..."
    
    if [ ! -f "$CONFIG_DIR/Project.config.txt" ]; then
        ((TOTAL_CHECKS++))
        log_error "Project.config.txt not found"
        return 1
    fi
    
    ((TOTAL_CHECKS++))
    log_success "Project.config.txt found"
    
    # Validate required config keys
    local config_file="$CONFIG_DIR/Project.config.txt"
    local required_keys=("SourceAssetsPath" "DatabasePath" "ProjectPath" "ProjectGUID")
    
    for key in "${required_keys[@]}"; do
        ((TOTAL_CHECKS++))
        if grep -q "^${key}=" "$config_file"; then
            log_success "Config key found: $key"
        else
            log_warning "Config key missing or malformed: $key"
        fi
    done
}

# ==============================================================================
# JSON File Validation
# ==============================================================================

validate_json_file() {
    local file=$1
    ((TOTAL_CHECKS++))
    
    if ! command -v jq &> /dev/null; then
        log_warning "jq not installed, skipping JSON validation for $file"
        return 0
    fi
    
    if jq empty "$file" 2>/dev/null; then
        log_success "Valid JSON: $(basename "$file")"
    else
        log_error "Invalid JSON: $file"
        return 1
    fi
}

validate_all_json_files() {
    log_info "Validating JSON files..."
    
    find "$SCENES_DIR" -name "*.scene" 2>/dev/null | while read -r file; do
        validate_json_file "$file"
    done
    
    # find "$CONFIG_DIR" -name "*.json" 2>/dev/null | while read -r file; do
    #     validate_json_file "$file"
    # done
}

# ==============================================================================
# Material Asset Validation
# ==============================================================================

validate_material_assets() {
    log_info "Validating material assets..."
    
    if [ ! -d "$MATERIALS_DIR" ]; then
        ((TOTAL_CHECKS++))
        log_warning "Materials directory not found"
        return 0
    fi
    
    local mat_count=0
    local orphaned_count=0
    
    while IFS= read -r -d '' mat_file; do
        ((TOTAL_CHECKS++))
        ((mat_count++))

        # Check for corresponding .meta file
        local meta_file="${mat_file}.meta"
        if [ ! -f "$meta_file" ]; then
            log_warning "Missing .meta file: $(basename "$mat_file")"
            ((orphaned_count++))
        else
            log_success "Material with .meta: $(basename "$mat_file")"
        fi

        # Validate material file format (basic)
        if ! validate_json_file "$mat_file"; then
            log_error "Malformed material file: $mat_file"
        fi
    done < <(find "$MATERIALS_DIR" -type f -name "*.mat" -print0)

    if [ "$mat_count" -eq 0 ]; then
        ((TOTAL_CHECKS++))
        log_warning "No .mat files found in $MATERIALS_DIR"
    fi

    if [ "$orphaned_count" -gt 0 ]; then
        log_warning "Materials missing .meta files: $orphaned_count"
    fi

    return 0
}

# ==============================================================================
# Shader Validation
# ==============================================================================

validate_shader_files() {
    log_info "Validating shader files..."
    
    if [ ! -d "$SHADERS_DIR" ]; then
        ((TOTAL_CHECKS++))
        log_warning "Shaders directory not found"
        return 0
    fi
    
    local glsl_count=0
    local shader_count=0
    local invalid_count=0
    
    # Check vertex shaders
        while IFS= read -r -d '' shader_file; do
        ((TOTAL_CHECKS++))
        ((shader_count++))

        if [ ! -s "$shader_file" ]; then
            log_error "Empty shader file: $shader_file"
            ((invalid_count++))
            continue
        fi

        # Basic sanity check: most shaders should have a main() entry point
        if grep -Eq 'void[[:space:]]+main[[:space:]]*\(' "$shader_file"; then
            log_success "Shader OK: $(basename "$shader_file")"
        else
            log_warning "No main() found in shader: $(basename "$shader_file")"
        fi
    done < <(find "$SHADERS_DIR" -type f \( \
        -name "*.glsl" -o -name "*.vert" -o -name "*.frag" -o -name "*.comp" -o \
        -name "*.geom" -o -name "*.tesc" -o -name "*.tese" \
    \) -print0)

    if [ "$shader_count" -eq 0 ]; then
        ((TOTAL_CHECKS++))
        log_warning "No shader files found in $SHADERS_DIR"
    fi

    if [ "$invalid_count" -gt 0 ]; then
        return 1
    fi

    return 0
}

# ==============================================================================
# Texture Validation
# ==============================================================================

validate_texture_files() {
    log_info "Validating texture files..."
    
    if [ ! -d "$TEXTURES_DIR" ]; then
        ((TOTAL_CHECKS++))
        log_warning "Textures directory not found"
        return 0
    fi
    
    local texture_count=0
    
    while IFS= read -r -d '' tex_file; do
        ((TOTAL_CHECKS++))
        ((texture_count++))
        
        if [ -f "$tex_file" ] && [ -s "$tex_file" ]; then
            log_success "Texture file valid: $(basename "$tex_file")"
        else
            log_error "Invalid or empty texture: $tex_file"
        fi
    done < <(find "$TEXTURES_DIR" -type f \( -name "*.png" -o -name "*.jpg" -o -name "*.dds" \) -print0)
    
    if [ "$texture_count" -eq 0 ]; then
        ((TOTAL_CHECKS++))
        log_warning "No texture files found"
    fi
}

# ==============================================================================
# Scene File Validation
# ==============================================================================

validate_scene_files() {
    log_info "Validating scene files..."
    
    if [ ! -d "$SCENES_DIR" ]; then
        ((TOTAL_CHECKS++))
        log_warning "Scenes directory not found"
        return 0
    fi
    
    local scene_count=0
    local invalid_count=0
    
    while IFS= read -r -d '' scene_file; do
        ((TOTAL_CHECKS++))
        ((scene_count++))

        if [ ! -s "$scene_file" ]; then
            log_error "Empty scene file: $scene_file"
            ((invalid_count++))
            continue
        fi

        # Scene files are JSON-like in this project
        if ! validate_json_file "$scene_file"; then
            log_error "Malformed scene file: $scene_file"
            ((invalid_count++))
        fi
        
    done < <(find "$SCENES_DIR" -type f -name "*.scene" -print0)
    
    if [ "$scene_count" -eq 0 ]; then
        ((TOTAL_CHECKS++))
        log_warning "No scene files found"
    fi

    if [ "$invalid_count" -gt 0 ]; then
        return 1
    fi

    return 0
}

# ==============================================================================
# Resource Database Validation
# ==============================================================================

validate_resource_database() {
    log_info "Validating resource database..."
    
    if [ ! -d "$ERMINE_GAME_DB" ]; then
        ((TOTAL_CHECKS++))
        log_warning "Resource database not found at: $ERMINE_GAME_DB"
        return 0
    fi
    
    ((TOTAL_CHECKS++))
    log_success "Resource database found"
    
    # Check for required database structure
    local required_subdirs=(
        "12345678-1234-1234-1234-123456789ABC/Browser.dbase"
        "12345678-1234-1234-1234-123456789ABC/Windows.platform/Data"
    )
    
    for subdir in "${required_subdirs[@]}"; do
        ((TOTAL_CHECKS++))
        if [ -d "$ERMINE_GAME_DB/$subdir" ]; then
            log_success "Database structure valid: $subdir"
        else
            log_warning "Database missing directory: $subdir"
        fi
    done
    
    # Check resource_database.txt
    local resource_db="$ERMINE_GAME_DB/12345678-1234-1234-1234-123456789ABC/Browser.dbase/resource_database.txt"
    if [ -f "$resource_db" ]; then
        ((TOTAL_CHECKS++))
        log_success "resource_database.txt found"
        
        # Count resources
        local resource_count=$(grep -c "RESOURCE_START" "$resource_db" 2>/dev/null || echo 0)
        log_info "Total resources in database: $resource_count"
    else
        ((TOTAL_CHECKS++))
        log_warning "resource_database.txt not found"
    fi
}

# ==============================================================================
# Meta File Consistency
# ==============================================================================

validate_meta_files() {
    log_info "Validating .meta file consistency..."
    
    # Find all asset files that should have .meta
    find "$MATERIALS_DIR" -name "*.mat" 2>/dev/null | while read -r asset_file; do
        ((TOTAL_CHECKS++))
        local meta_file="${asset_file}.meta"
        
        if [ -f "$meta_file" ]; then
            # Validate meta file format
            if validate_json_file "$meta_file"; then
                log_success "Valid .meta file: $(basename "$asset_file")"
            fi
        else
            log_warning "Missing .meta for asset: $(basename "$asset_file")"
        fi
    done
}

# ==============================================================================
# Duplicate Check
# ==============================================================================

check_duplicate_files() {
    log_info "Checking for duplicate asset files..."
    
    local duplicates=0
    
    find "$RESOURCES_DIR" -type f -name "*.png" -o -name "*.jpg" -o -name "*.mat" 2>/dev/null | \
    sort | uniq -d | while read -r dup_file; do
        ((TOTAL_CHECKS++))
        log_warning "Duplicate file detected: $dup_file"
        ((duplicates++))
    done
}

# ==============================================================================
# Permission Check
# ==============================================================================

check_file_permissions() {
    log_info "Validating file permissions..."
    
    # Check that asset directories are readable
    local asset_dirs=("$MATERIALS_DIR" "$SHADERS_DIR" "$TEXTURES_DIR" "$SCENES_DIR")
    
    for dir in "${asset_dirs[@]}"; do
        if [ -d "$dir" ]; then
            ((TOTAL_CHECKS++))
            if [ -r "$dir" ]; then
                log_success "Directory readable: $dir"
            else
                log_error "Directory not readable: $dir"
            fi
        fi
    done
}

# ==============================================================================
# Dependency Check (Material References)
# ==============================================================================

check_material_dependencies() {
    log_info "Checking material texture references..."
    
    if ! command -v jq &> /dev/null; then
        ((TOTAL_CHECKS++))
        log_warning "jq not installed, skipping dependency validation"
        return 0
    fi
    
    find "$MATERIALS_DIR" -name "*.mat" 2>/dev/null | while read -r mat_file; do
        ((TOTAL_CHECKS++))
        
        # Extract texture references from material
        local textures=$(jq -r '.parameters[]?.texture // empty' "$mat_file" 2>/dev/null || echo "")
        
        while IFS= read -r tex_ref; do
            [ -z "$tex_ref" ] && continue
            
            # Check if referenced texture exists
            if [ -f "$RESOURCES_DIR/$tex_ref" ] || [ -f "$tex_ref" ]; then
                log_success "Texture reference valid: $tex_ref"
            else
                log_warning "Texture reference broken in $(basename "$mat_file"): $tex_ref"
            fi
        done <<< "$textures"
    done
}

# ==============================================================================
# Report Generation
# ==============================================================================

print_report() {
    echo ""
    echo "================================================================================"
    echo "                    VALIDATION REPORT - ASSETS & CONFIG                        "
    echo "================================================================================"
    echo ""
    echo "Total Checks:     $TOTAL_CHECKS"
    echo -e "${GREEN}Passed:           $PASSED_CHECKS${NC}"
    echo -e "${RED}Failed:           $FAILED_CHECKS${NC}"
    echo -e "${YELLOW}Warnings:         $WARNINGS${NC}"
    echo ""
    
    if [ $FAILED_CHECKS -eq 0 ]; then
        echo -e "${GREEN}✓ All validations passed!${NC}"
        echo ""
        return 0
    else
        echo -e "${RED}✗ Some validations failed. Please review the errors above.${NC}"
        echo ""
        return 1
    fi
}

# ==============================================================================
# Main Execution
# ==============================================================================

main() {
    echo "================================================================================"
    echo "          Ermine Engine CI - Asset, Data, Configuration Validation"
    echo "================================================================================"
    echo ""
    log_info "Project Root: $PROJECT_ROOT"
    log_info "Resources Dir: $RESOURCES_DIR"
    echo ""
    
    # Run all validations
    validate_directory_structure
    validate_build_file_presence
    # validate_config_files
    validate_all_json_files
    validate_material_assets
    validate_shader_files
    validate_texture_files
    validate_scene_files
    validate_resource_database
    validate_meta_files
    check_duplicate_files
    check_file_permissions
    check_material_dependencies
    
    # Print final report
    print_report
    # read -r -n 1 -s -p "Press any key to continue..." < /dev/tty
    # echo
}

# Execute main function and exit with appropriate code
main
status=$?
exit $status
