#!/usr/bin/env python3
"""
Ermine Works comprehensive asset usage analysis for all asset types.
Checks: Audio, Fonts, Materials, Models, Prefabs, Scenes, Shaders, Textures, Videos

Enhanced features:
- Scans code files for asset references
- File size analysis and space savings calculations
- JSON export for further processing
- Modular API for programmatic use
"""

import json
import os
import re
from pathlib import Path
from collections import defaultdict
from typing import Dict, Set, Tuple, List, Any

# Get the base directory (Ermine-Engine) and then Resources subdirectory
RESOURCES_DIR = Path("Ermine-Engine") / "Resources"
AUDIO_DIR = RESOURCES_DIR / "Audio"
FONTS_DIR = RESOURCES_DIR / "Fonts"
MATERIALS_DIR = RESOURCES_DIR / "Materials"
MODELS_DIR = RESOURCES_DIR / "Models"
PREFABS_DIR = RESOURCES_DIR / "Prefabs"
SCENES_DIR = RESOURCES_DIR / "Scenes"
SHADERS_DIR = RESOURCES_DIR / "Shaders"
TEXTURES_DIR = RESOURCES_DIR / "Textures"
VIDEOS_DIR = RESOURCES_DIR / "Videos"

# Map asset types to their base directories for deletion
ASSET_DIRS: Dict[str, Path] = {
    'audio': AUDIO_DIR,
    'fonts': FONTS_DIR,
    'materials': MATERIALS_DIR,
    'models': MODELS_DIR,
    'prefabs': PREFABS_DIR,
    'scenes': SCENES_DIR,
    'shaders': SHADERS_DIR,
    'textures': TEXTURES_DIR,
    'videos': VIDEOS_DIR,
}

CODE_DIRS = [
    Path("Ermine-ScriptAssembly"),
    Path("Ermine-ScriptSandbox"),
    Path("Ermine-Engine/src"),
    Path("Ermine-Editor/src"),
    Path("Ermine-Game/src"),
]


def get_file_size(file_path: Path) -> int:
    """Get file size in bytes."""
    try:
        return file_path.stat().st_size
    except OSError:
        return 0


def format_size(bytes_size: int) -> str:
    """Format bytes to human readable format."""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if bytes_size < 1024.0:
            return f"{bytes_size:.1f} {unit}"
        bytes_size /= 1024.0
    return f"{bytes_size:.1f} GB"


def scan_code_for_assets() -> Dict[str, Set[str]]:
    """Scan code files for asset references using regex patterns."""
    refs = {
        'audio': set(),
        'fonts': set(),
        'materials': set(),
        'models': set(),
        'prefabs': set(),
        'scenes': set(),
        'shaders': set(),
        'textures': set(),
        'videos': set(),
    }
    
    # Asset reference patterns (common ways assets are referenced in code)
    patterns = {
        'audio': [
            r'["\']([^"\']*\.(?:mp3|wav|ogg|m4a))["\']',
            r'LoadAudio\(["\']([^"\']+)["\']',
            r'AudioClip\.Load\(["\']([^"\']+)["\']',
        ],
        'fonts': [
            r'["\']([^"\']*\.(?:ttf|otf|fnt))["\']',
            r'LoadFont\(["\']([^"\']+)["\']',
            r'Font\.Load\(["\']([^"\']+)["\']',
        ],
        'materials': [
            r'["\']([^"\']*\.mat)["\']',
            r'LoadMaterial\(["\']([^"\']+)["\']',
            r'Material\.Load\(["\']([^"\']+)["\']',
        ],
        'models': [
            r'["\']([^"\']*\.(?:fbx|gltf|glb|obj))["\']',
            r'LoadModel\(["\']([^"\']+)["\']',
            r'Model\.Load\(["\']([^"\']+)["\']',
        ],
        'prefabs': [
            r'["\']([^"\']*\.prefab)["\']',
            r'LoadPrefab\(["\']([^"\']+)["\']',
            r'Instantiate\(["\']([^"\']*\.prefab)["\']',
        ],
        'scenes': [
            r'["\']([^"\']*\.scene)["\']',
            r'LoadScene\(["\']([^"\']+)["\']',
            r'SceneManager\.LoadScene\(["\']([^"\']+)["\']',
        ],
        'shaders': [
            r'["\']([^"\']*\.(?:glsl|hlsl|shader))["\']',
            r'LoadShader\(["\']([^"\']+)["\']',
            r'Shader\.Find\(["\']([^"\']+)["\']',
        ],
        'textures': [
            r'["\']([^"\']*\.(?:png|jpg|jpeg|tga|bmp|hdr))["\']',
            r'LoadTexture\(["\']([^"\']+)["\']',
            r'Texture2D\.Load\(["\']([^"\']+)["\']',
        ],
        'videos': [
            r'["\']([^"\']*\.(?:mp4|webm|mov|avi|mpeg|mpg|wmv|flv))["\']',
            r'LoadVideo\(["\']([^"\']+)["\']',
            r'VideoPlayer\.url\s*=\s*["\']([^"\']+)["\']',
        ],
    }
    
    for code_dir in CODE_DIRS:
        if not code_dir.exists():
            continue
            
        for file_path in code_dir.rglob("*"):
            if file_path.is_file() and file_path.suffix in ['.cs', '.cpp', '.h', '.py', '.js']:
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        
                    for asset_type, asset_patterns in patterns.items():
                        for pattern in asset_patterns:
                            matches = re.findall(pattern, content, re.IGNORECASE)
                            for match in matches:
                                # Extract filename from path
                                filename = Path(match).name
                                if filename:
                                    refs[asset_type].add(filename)
                                    
                except Exception as e:
                    print(f"  WARNING: Error reading {file_path}: {e}")
    
    return refs


def get_all_files_with_sizes(directory: Path, patterns: List[str]) -> Dict[str, int]:
    """Get all files matching patterns with their sizes."""
    files = {}
    if not directory.exists():
        return files
    
    for pattern in patterns:
        for file in directory.glob(pattern):
            if file.is_file():
                files[file.name] = get_file_size(file)
    return files


def scan_scenes():
    """Scan all scene files for asset references."""
    refs = {
        'audio': defaultdict(set),
        'fonts': defaultdict(set),
        'materials': defaultdict(set),
        'models': defaultdict(set),
        'prefabs': defaultdict(set),
        'shaders': defaultdict(set),
        'textures': defaultdict(set),
        'videos': defaultdict(set),
    }
    
    if not SCENES_DIR.exists():
        return refs

    audio_exts = (".mp3", ".wav", ".ogg", ".m4a")
    video_exts = (".mp4", ".webm", ".mov", ".avi", ".mpeg", ".mpg", ".wmv", ".flv")
    
    for scene_file in SCENES_DIR.glob("*.scene"):
        try:
            with open(scene_file, 'r') as f:
                scene_data = json.load(f)
            
            entities = scene_data.get("entities", [])
            
            for entity in entities:
                components = entity.get("components", {})
                
                # Models
                if "ModelComponent" in components:
                    model = components["ModelComponent"].get("model", "")
                    if model:
                        refs['models'][model].add(scene_file.name)
                
                # Audio
                if "AudioComponent" in components:
                    audio_comp = components["AudioComponent"]
                    sound_name = audio_comp.get("soundName", "")
                    if sound_name:
                        refs['audio'][sound_name].add(scene_file.name)
                    variations = audio_comp.get("soundVariations", [])
                    for var in variations:
                        if var:
                            refs['audio'][Path(var).name].add(scene_file.name)
                
                if "GlobalAudioComponent" in components:
                    audio_comp = components["GlobalAudioComponent"]
                    for sound_type in ['music', 'sfx', 'ambience']:
                        for item in audio_comp.get(sound_type, []):
                            path = item.get("path", "")
                            if path:
                                refs['audio'][Path(path).name].add(scene_file.name)
                
                # Materials
                if "Material" in components:
                    mat_guid = components["Material"].get("guid", "")
                    if mat_guid:
                        refs['materials'][mat_guid].add(scene_file.name)
                
                # Videos
                if "VideoComponent" in components:
                    video = components["VideoComponent"].get("videoPath", "")
                    if video:
                        refs['videos'][Path(video).name].add(scene_file.name)

            # Fallback: generic scan for any string paths in the JSON that look like audio/video files.
            # This will catch things like custom fields such as the provided examples:
            # "../Resources/Audio/1stClue.wav" or "IntroCinematic.mpeg".
            def _walk_and_collect(obj):
                if isinstance(obj, dict):
                    for v in obj.values():
                        _walk_and_collect(v)
                elif isinstance(obj, list):
                    for item in obj:
                        _walk_and_collect(item)
                elif isinstance(obj, str):
                    lower = obj.lower()
                    if lower.endswith(audio_exts):
                        refs['audio'][Path(obj).name].add(scene_file.name)
                    if lower.endswith(video_exts):
                        refs['videos'][Path(obj).name].add(scene_file.name)

            _walk_and_collect(scene_data)

        except json.JSONDecodeError:
            print(f"  WARNING: Invalid JSON in {scene_file.name}")
        except Exception as e:
            print(f"  WARNING: Error reading {scene_file.name}: {e}")
    
    return refs


def scan_materials_for_shaders_textures():
    """Scan materials for shader and texture references."""
    shaders_refs = defaultdict(set)
    textures_refs = defaultdict(set)
    
    if not MATERIALS_DIR.exists():
        return shaders_refs, textures_refs
    
    for mat_file in MATERIALS_DIR.glob("*.mat"):
        try:
            with open(mat_file, 'r') as f:
                mat_data = json.load(f)
            
            # Check for shader references
            if "shader" in mat_data:
                shader = mat_data.get("shader", "")
                if shader:
                    shaders_refs[Path(shader).name].add(mat_file.name)
            
            # Check textures
            parameters = mat_data.get("parameters", {})
            for param_name, param_data in parameters.items():
                if param_data.get("type") == "texture2d":
                    texture_path = param_data.get("value", "")
                    if texture_path:
                        texture_name = Path(texture_path).name
                        textures_refs[texture_name].add(mat_file.name)
        except json.JSONDecodeError:
            pass
        except Exception:
            pass
    
    return shaders_refs, textures_refs


def get_material_guid_map():
    """Build mapping of GUIDs to material filenames."""
    guid_to_file = {}
    
    if not MATERIALS_DIR.exists():
        return guid_to_file
    
    for meta_file in MATERIALS_DIR.glob("*.mat.meta"):
        try:
            with open(meta_file, 'r') as f:
                meta_data = json.load(f)
                if "guid" in meta_data:
                    guid = meta_data["guid"]
                    mat_filename = meta_file.stem
                    guid_to_file[guid] = mat_filename
        except:
            pass
    
    return guid_to_file


def find_orphan_meta_files() -> List[Path]:
    """Find .meta files in Resources that do not have a corresponding asset file.

    Example: 'foo.wav.meta' is considered orphaned if 'foo.wav' does not exist.
    """
    orphan_metas: List[Path] = []

    if not RESOURCES_DIR.exists():
        return orphan_metas

    for meta_path in RESOURCES_DIR.rglob("*.meta"):
        # Strip the trailing '.meta' extension to find the underlying asset
        asset_path = meta_path.with_suffix("")
        if not asset_path.exists():
            orphan_metas.append(meta_path)

    return orphan_metas


def get_all_files(directory, patterns):
    """Get all files matching patterns in a directory."""
    files = set()
    if not directory.exists():
        return files
    
    for pattern in patterns:
        for file in directory.glob(pattern):
            if file.is_file():
                files.add(file.name)
    return files


def analyze_asset_usage() -> Dict[str, Any]:
    """Main analysis function that returns comprehensive asset usage data."""
    
    # Initialize data structure
    analysis_data = {
        'audio': {'all': {}, 'referenced': set(), 'unused': {}, 'total_size': 0, 'unused_size': 0},
        'fonts': {'all': {}, 'referenced': set(), 'unused': {}, 'total_size': 0, 'unused_size': 0},
        'materials': {'all': {}, 'referenced': set(), 'unused': {}, 'total_size': 0, 'unused_size': 0},
        'models': {'all': {}, 'referenced': set(), 'unused': {}, 'total_size': 0, 'unused_size': 0},
        'prefabs': {'all': {}, 'referenced': set(), 'unused': {}, 'total_size': 0, 'unused_size': 0},
        'scenes': {'all': {}, 'referenced': set(), 'unused': {}, 'total_size': 0, 'unused_size': 0},
        'shaders': {'all': {}, 'referenced': set(), 'unused': {}, 'total_size': 0, 'unused_size': 0},
        'textures': {'all': {}, 'referenced': set(), 'unused': {}, 'total_size': 0, 'unused_size': 0},
        'videos': {'all': {}, 'referenced': set(), 'unused': {}, 'total_size': 0, 'unused_size': 0},
    }
    
    # Get all files with sizes
    analysis_data['audio']['all'] = get_all_files_with_sizes(AUDIO_DIR, ["*.mp3", "*.wav", "*.ogg", "*.m4a"])
    analysis_data['fonts']['all'] = get_all_files_with_sizes(FONTS_DIR, ["*.ttf", "*.otf", "*.fnt"])
    analysis_data['materials']['all'] = get_all_files_with_sizes(MATERIALS_DIR, ["*.mat"])
    analysis_data['models']['all'] = get_all_files_with_sizes(MODELS_DIR, ["*.fbx", "*.gltf", "*.glb", "*.obj"])
    analysis_data['prefabs']['all'] = get_all_files_with_sizes(PREFABS_DIR, ["*.prefab", "*.json"])
    analysis_data['scenes']['all'] = get_all_files_with_sizes(SCENES_DIR, ["*.scene"])
    analysis_data['shaders']['all'] = get_all_files_with_sizes(SHADERS_DIR, ["*.glsl", "*.hlsl", "*.shader", "*.mat"])
    analysis_data['textures']['all'] = get_all_files_with_sizes(TEXTURES_DIR, ["*.png", "*.jpg", "*.jpeg", "*.tga", "*.bmp", "*.hdr"])
    analysis_data['videos']['all'] = get_all_files_with_sizes(VIDEOS_DIR, ["*.mp4", "*.webm", "*.mov", "*.avi", "*.mpeg", "*.mpg", "*.wmv", "*.flv"])
    
    # Calculate total sizes
    for asset_type in analysis_data:
        analysis_data[asset_type]['total_size'] = sum(analysis_data[asset_type]['all'].values())
    
    # Scan for references
    print("Scanning scenes for asset references...")
    scene_refs = scan_scenes()
    
    print("Scanning materials for shader/texture references...")
    shader_refs, texture_refs = scan_materials_for_shaders_textures()
    
    print("Scanning code files for asset references...")
    code_refs = scan_code_for_assets()
    
    print("Building GUID mappings...")
    guid_map = get_material_guid_map()
    
    # Process scene references
    for asset_type in ['audio', 'materials', 'models', 'videos']:
        for asset_name, scenes in scene_refs[asset_type].items():
            if asset_type == 'materials' and asset_name in guid_map:
                asset_name = guid_map[asset_name]
            analysis_data[asset_type]['referenced'].add(asset_name)
    
    # Process material references (shaders and textures)
    for shader_name, materials in shader_refs.items():
        analysis_data['shaders']['referenced'].add(shader_name)
    for texture_name, materials in texture_refs.items():
        analysis_data['textures']['referenced'].add(texture_name)
    
    # Process code references
    for asset_type in code_refs:
        analysis_data[asset_type]['referenced'].update(code_refs[asset_type])
    
    # Normalize references and calculate unused assets and sizes
    for asset_type in analysis_data:
        all_files = set(analysis_data[asset_type]['all'].keys())
        # Only keep references that actually correspond to existing files
        analysis_data[asset_type]['referenced'] = {
            name for name in analysis_data[asset_type]['referenced'] if name in all_files
        }
        referenced = analysis_data[asset_type]['referenced']
        unused = all_files - referenced
        
        analysis_data[asset_type]['unused'] = {
            name: analysis_data[asset_type]['all'][name] for name in unused
        }
        analysis_data[asset_type]['unused_size'] = sum(
            analysis_data[asset_type]['unused'].values()
        )
    
    return analysis_data


def print_asset_report(analysis_data: Dict[str, Any]) -> None:
    """Print detailed asset usage report."""
    
    asset_types = {
        'audio': ('AUDIO', ["*.mp3", "*.wav", "*.ogg", "*.m4a"]),
        'fonts': ('FONTS', ["*.ttf", "*.otf", "*.fnt"]),
        'materials': ('MATERIALS', ["*.mat"]),
        'models': ('MODELS', ["*.fbx", "*.gltf", "*.glb", "*.obj"]),
        'prefabs': ('PREFABS', ["*.prefab", "*.json"]),
        'scenes': ('SCENES', ["*.scene"]),
        'shaders': ('SHADERS', ["*.glsl", "*.hlsl", "*.shader", "*.mat"]),
        'textures': ('TEXTURES', ["*.png", "*.jpg", "*.jpeg", "*.tga", "*.bmp", "*.hdr"]),
        'videos': ('VIDEOS', ["*.mp4", "*.webm", "*.mov", "*.avi"]),
    }
    
    for asset_type, (display_name, patterns) in asset_types.items():
        data = analysis_data[asset_type]
        all_files = data['all']
        referenced = data['referenced']
        unused = data['unused']
        
        print(f"\n\n{'='*80}")
        print(f"[{display_name}]")
        print(f"{'='*80}")
        
        print(f"Total {asset_type} files: {len(all_files)}")
        print(f"Total size: {format_size(data['total_size'])}")
        print(f"Referenced {asset_type}: {len(referenced)}")
        
        if all_files:
            usage_rate = len(referenced) / len(all_files) * 100 if all_files else 0
            print(f"Usage rate: {usage_rate:.1f}%")
            
            if unused:
                print(f"\n⚠️ UNUSED {display_name.upper()} ({len(unused)} files, {format_size(data['unused_size'])}):")
                for asset_name in sorted(unused.keys()):
                    size = unused[asset_name]
                    print(f"  - {asset_name} ({format_size(size)})")
            else:
                print(f"\n✓ All {asset_type} files are being used!")
        else:
            print(f"  No {asset_type} files found")


def print_summary_report(analysis_data: Dict[str, Any]) -> None:
    """Print summary of unused assets and space savings."""
    
    print(f"\n\n{'='*80}")
    print("SUMMARY REPORT")
    print(f"{'='*80}")
    
    total_assets = 0
    total_referenced = 0
    total_unused = 0
    total_size = 0
    total_unused_size = 0
    
    for asset_type, data in analysis_data.items():
        all_count = len(data['all'])
        ref_count = len(data['referenced'])
        unused_count = len(data['unused'])
        
        total_assets += all_count
        total_referenced += ref_count
        total_unused += unused_count
        total_size += data['total_size']
        total_unused_size += data['unused_size']
        
        if unused_count > 0:
            print(f"Unused {asset_type.capitalize()}: {unused_count} ({format_size(data['unused_size'])})")

    overall_usage = total_referenced / total_assets * 100 if total_assets > 0 else 0
    
    print(f"\nOVERALL STATISTICS:")
    print(f"Total Assets: {total_assets}")
    print(f"Referenced Assets: {total_referenced}")
    print(f"Unused Assets: {total_unused}")
    print(f"Overall Usage Rate: {overall_usage:.1f}%")
    print(f"Total Asset Size: {format_size(total_size)}")
    print(f"Potential Space Savings: {format_size(total_unused_size)}")
    
    if total_unused > 0:
        print(f"\nRECOMMENDATIONS:")
        print(f"• Consider removing {total_unused} unused assets to save {format_size(total_unused_size)}")
        print(f"• Review asset references in code and scenes before deletion")
        print(f"• Backup assets before cleanup")
    
    print(f"\nNOTES:")
    print(f"• Fonts, Prefabs, and Scenes may have additional references not detected")
    print(f"• Some assets may be loaded dynamically or referenced indirectly")


def export_to_json(analysis_data: Dict[str, Any], filename: str = "asset_analysis.json") -> None:
    """Export analysis data to JSON file."""
    try:
        with open(filename, 'w', encoding='utf-8') as f:
            json.dump(analysis_data, f, indent=2, ensure_ascii=False)
        print(f"\nAnalysis data exported to {filename}")
    except Exception as e:
        print(f"\nFailed to export to JSON: {e}")


def choose_category_and_delete_assets(analysis_data: Dict[str, Any]) -> None:
    """Allow user to choose categories, list unused assets with numbers, and delete by number.

    After finishing a category (pressing 'q' in the asset list), the user is
    returned to the category selection menu until they cancel there.
    """
    # Order and display names for menu
    asset_menu = [
        ('audio', 'Audio'),
        ('fonts', 'Fonts'),
        ('materials', 'Materials'),
        ('models', 'Models'),
        ('prefabs', 'Prefabs'),
        ('scenes', 'Scenes'),
        ('shaders', 'Shaders'),
        ('textures', 'Textures'),
        ('videos', 'Videos'),
    ]

    print(f"\n{'='*80}")
    print("DELETE UNUSED ASSETS")
    print(f"{'='*80}")

    while True:
        # Filter to only categories that actually have unused assets
        available_categories = [
            (key, label)
            for key, label in asset_menu
            if analysis_data.get(key, {}).get('unused')
        ]

        if not available_categories:
            print("\nNo unused assets found in any category. Nothing more to delete.")
            return

        print("\nSelect a category to clean up (only categories with unused assets are shown):\n")
        for idx, (key, label) in enumerate(available_categories, start=1):
            data = analysis_data[key]
            unused_count = len(data['unused'])
            unused_size = data['unused_size']
            print(f"{idx}. {label} - {unused_count} unused files ({format_size(unused_size)})")

        choice = input("\nEnter category number to manage (or 'q' to stop deleting): ").strip().lower()
        if choice == 'q':
            print("Stopping deletion; returning to main flow.")
            return
        if not choice.isdigit():
            print("Invalid selection. Please enter a valid number or 'q' to stop.")
            continue

        idx = int(choice)
        if not (1 <= idx <= len(available_categories)):
            print("Invalid number. Please choose one of the listed category numbers.")
            continue

        asset_type, label = available_categories[idx - 1]

        base_dir = ASSET_DIRS.get(asset_type)
        if base_dir is None or not base_dir.exists():
            print(f"\nBase directory for {label} does not exist. Nothing to delete.")
            continue

        # Build and show numbered list of unused assets
        unused_items = sorted(
            analysis_data[asset_type]['unused'].items(),
            key=lambda kv: kv[0].lower(),
        )

        if not unused_items:
            print(f"\nNo unused {label} assets to delete.")
            continue

        print(f"\nYou selected: {label}")
        print(f"Unused {label} assets in '{base_dir}':\n")

        for i, (name, size) in enumerate(unused_items, start=1):
            print(f"{i}. {name} ({format_size(size)})")

        print("\nType the number of the asset you want to delete.")
        print("You can delete multiple assets one by one.")
        print("Enter 'q' when you are done with this category and want to go back to category selection.")

        # Per-category deletion loop
        while True:
            choice = input("\nEnter asset number to delete (or 'q' to go back to categories): ").strip().lower()
            if choice == 'q':
                print(f"Finished deleting assets for {label}. Returning to category selection.")
                break

            if not choice.isdigit():
                print("Please enter a valid number or 'q'.")
                continue

            asset_idx = int(choice)
            if not (1 <= asset_idx <= len(unused_items)):
                print("Number out of range. Please choose one of the listed asset numbers.")
                continue

            name, size = unused_items[asset_idx - 1]

            # Asset may have already been deleted in this session
            if name not in analysis_data[asset_type]['unused']:
                print(f"Asset '{name}' has already been deleted or is no longer marked as unused.")
                continue

            asset_path = base_dir / name

            try:
                if asset_path.exists():
                    os.remove(asset_path)
                    print(f"  Deleted asset: {asset_path}")

                    # Also delete matching .meta file if it exists (same name + '.meta')
                    meta_path = asset_path.with_suffix(asset_path.suffix + ".meta")
                    if meta_path.exists():
                        try:
                            os.remove(meta_path)
                            print(f"  Deleted meta:  {meta_path}")
                        except Exception as e:
                            print(f"  ERROR deleting meta file {meta_path}: {e}")

                    # Update analysis data so it stays consistent
                    analysis_data[asset_type]['all'].pop(name, None)
                    analysis_data[asset_type]['unused'].pop(name, None)
                    analysis_data[asset_type]['total_size'] -= size
                    analysis_data[asset_type]['unused_size'] -= size
                else:
                    print(f"  File not found on disk: {asset_path}")
            except Exception as e:
                print(f"  ERROR deleting {asset_path}: {e}")



def main():
    """Main entry point."""
    print("="*80)
    print("ERMINE WORKS COMPREHENSIVE ASSET USAGE ANALYSIS")
    print("="*80)
    print("Enhanced with code scanning, file sizes, and detailed reporting")
    
    # Run analysis
    analysis_data = analyze_asset_usage()
    
    # Print detailed report
    print_asset_report(analysis_data)
    
    # Print summary
    print_summary_report(analysis_data)

    # Allow user to interactively delete unused assets
    while True:
        resp = input("\nDo you want to delete unused assets from a category? [y/n]: ").strip().lower()
        if resp in {'y', 'n'}:
            break
        print("Please answer with 'y' or 'n'.")

    if resp == 'y':
        choose_category_and_delete_assets(analysis_data)

    # Check for orphan .meta files (no corresponding asset)
    orphan_metas = find_orphan_meta_files()
    if orphan_metas:
        print(f"\n{'='*80}")
        print("ORPHAN .META FILES")
        print(f"{'='*80}")
        print(f"Found {len(orphan_metas)} .meta files with no corresponding asset:")
        for p in sorted(orphan_metas):
            print(f"  - {p}")

        while True:
            resp_meta = input("\nDo you want to delete ALL orphan .meta files listed above? [y/n]: ").strip().lower()
            if resp_meta in {'y', 'n'}:
                break
            print("Please answer with 'y' or 'n'.")

        if resp_meta == 'y':
            for meta_path in orphan_metas:
                try:
                    if meta_path.exists():
                        os.remove(meta_path)
                        print(f"  Deleted orphan meta: {meta_path}")
                except Exception as e:
                    print(f"  ERROR deleting orphan meta {meta_path}: {e}")
    
    # Export to JSON
    # export_to_json(analysis_data)


if __name__ == "__main__":
    main()
