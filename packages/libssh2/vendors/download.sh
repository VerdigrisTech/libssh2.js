#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENDOR_DIR="$SCRIPT_DIR/vendor"
PACKAGE_JSON="$SCRIPT_DIR/package.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if required commands are available
check_dependencies() {
    local deps=("curl" "jq" "sha256sum" "tar")
    
    for dep in "${deps[@]}"; do
        if ! command -v "$dep" &> /dev/null; then
            log_error "$dep is required but not installed."
            exit 1
        fi
    done
}

# Verify SHA256 hash
verify_hash() {
    local file="$1"
    local expected_hash="$2"
    
    local actual_hash
    if command -v sha256sum &> /dev/null; then
        actual_hash=$(sha256sum "$file" | cut -d' ' -f1)
    elif command -v shasum &> /dev/null; then
        actual_hash=$(shasum -a 256 "$file" | cut -d' ' -f1)
    else
        log_error "Neither sha256sum nor shasum available for hash verification"
        return 1
    fi
    
    if [ "$actual_hash" = "$expected_hash" ]; then
        log_success "Hash verification passed for $(basename "$file")"
        return 0
    else
        log_error "Hash verification failed for $(basename "$file")"
        log_error "Expected: $expected_hash"
        log_error "Actual:   $actual_hash"
        return 1
    fi
}

# Download file with progress
download_file() {
    local url="$1"
    local output_file="$2"
    
    log_info "Downloading $(basename "$output_file") from $url"
    
    if curl --fail --location --progress-bar --output "$output_file" "$url"; then
        log_success "Downloaded $(basename "$output_file")"
        return 0
    else
        log_error "Failed to download $(basename "$output_file")"
        return 1
    fi
}

# Extract archive
extract_archive() {
    local archive="$1"
    local extract_dir="$2"
    local vendor_name="$3"
    
    log_info "Extracting $(basename "$archive") to $extract_dir"
    
    # Remove existing directory if it exists
    if [ -d "$extract_dir/$vendor_name" ]; then
        log_warning "Removing existing $vendor_name directory"
        rm -rf "$extract_dir/$vendor_name"
    fi
    
    case "$archive" in
        *.tar.gz|*.tgz)
            if tar -xzf "$archive" -C "$extract_dir"; then
                # Find the extracted directory and rename it to the vendor name
                local extracted_dir
                extracted_dir=$(tar -tzf "$archive" | head -1 | cut -d/ -f1)
                if [ -d "$extract_dir/$extracted_dir" ] && [ "$extracted_dir" != "$vendor_name" ]; then
                    mv "$extract_dir/$extracted_dir" "$extract_dir/$vendor_name"
                fi
                log_success "Extracted $(basename "$archive")"
                return 0
            fi
            ;;
        *.tar.bz2|*.tbz2)
            if tar -xjf "$archive" -C "$extract_dir"; then
                local extracted_dir
                extracted_dir=$(tar -tjf "$archive" | head -1 | cut -d/ -f1)
                if [ -d "$extract_dir/$extracted_dir" ] && [ "$extracted_dir" != "$vendor_name" ]; then
                    mv "$extract_dir/$extracted_dir" "$extract_dir/$vendor_name"
                fi
                log_success "Extracted $(basename "$archive")"
                return 0
            fi
            ;;
        *.zip)
            if unzip -q "$archive" -d "$extract_dir"; then
                log_success "Extracted $(basename "$archive")"
                return 0
            fi
            ;;
        *)
            log_error "Unsupported archive format: $archive"
            return 1
            ;;
    esac
    
    log_error "Failed to extract $(basename "$archive")"
    return 1
}

# Process a single vendor
process_vendor() {
    local vendor_name="$1"
    local vendor_config="$2"
    
    local url
    local sha256
    
    url=$(echo "$vendor_config" | jq -r '.url')
    sha256=$(echo "$vendor_config" | jq -r '.sha256 // empty')
    
    if [ "$url" = "null" ] || [ -z "$url" ]; then
        log_error "No URL specified for vendor $vendor_name"
        return 1
    fi
    
    local filename
    filename=$(basename "$url")
    local archive_path="$VENDOR_DIR/$filename"
    
    log_info "Processing vendor: $vendor_name"
    
    # Download the file
    if ! download_file "$url" "$archive_path"; then
        return 1
    fi
    
    # Verify hash if provided
    if [ -n "$sha256" ] && [ "$sha256" != "null" ]; then
        if ! verify_hash "$archive_path" "$sha256"; then
            log_error "Removing corrupted file: $archive_path"
            rm -f "$archive_path"
            return 1
        fi
    else
        log_warning "No hash provided for $vendor_name, skipping verification"
    fi
    
    # Extract the archive
    if ! extract_archive "$archive_path" "$VENDOR_DIR" "$vendor_name"; then
        return 1
    fi
    
    log_success "Successfully processed vendor: $vendor_name"
    return 0
}

# Main function
main() {
    log_info "Starting vendor download and extraction process"
    
    # Check dependencies
    check_dependencies
    
    # Create vendor directory if it doesn't exist
    mkdir -p "$VENDOR_DIR"
    
    # Check if package.json exists
    if [ ! -f "$PACKAGE_JSON" ]; then
        log_error "package.json not found at $PACKAGE_JSON"
        exit 1
    fi
    
    # Check if vendors section exists
    if ! jq -e '.vendors' "$PACKAGE_JSON" >/dev/null 2>&1; then
        log_error "No 'vendors' section found in package.json"
        exit 1
    fi
    
    # Get list of vendors
    local vendors
    vendors=$(jq -r '.vendors | keys[]' "$PACKAGE_JSON")
    
    if [ -z "$vendors" ]; then
        log_warning "No vendors defined in package.json"
        exit 0
    fi
    
    local failed_vendors=()
    local successful_vendors=()
    
    # Process each vendor
    while IFS= read -r vendor_name; do
        local vendor_config
        vendor_config=$(jq -c ".vendors[\"$vendor_name\"]" "$PACKAGE_JSON")
        
        if process_vendor "$vendor_name" "$vendor_config"; then
            successful_vendors+=("$vendor_name")
        else
            failed_vendors+=("$vendor_name")
        fi
        
        echo # Add blank line between vendors
    done <<< "$vendors"
    
    # Summary
    echo "========================================="
    log_info "Summary:"
    
    if [ ${#successful_vendors[@]} -gt 0 ]; then
        log_success "Successfully processed: ${successful_vendors[*]}"
    fi
    
    if [ ${#failed_vendors[@]} -gt 0 ]; then
        log_error "Failed to process: ${failed_vendors[*]}"
        exit 1
    else
        log_success "All vendors processed successfully!"
    fi
}

# Handle script arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [--help]"
        echo ""
        echo "Downloads and extracts vendor archives defined in package.json"
        echo ""
        echo "The script reads vendor configurations from the 'vendors' section"
        echo "of package.json and downloads/extracts them to the vendor/ directory."
        echo ""
        echo "Each vendor entry should have:"
        echo "  - url: Download URL for the archive"
        echo "  - sha256: (optional) SHA256 hash for verification"
        echo ""
        echo "Example package.json vendors section:"
        echo '{'
        echo '  "vendors": {'
        echo '    "zlib": {'
        echo '      "url": "https://www.zlib.net/zlib-1.3.1.tar.gz",'
        echo '      "sha256": "9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23"'
        echo '    }'
        echo '  }'
        echo '}'
        exit 0
        ;;
    "")
        main
        ;;
    *)
        log_error "Unknown argument: $1"
        echo "Use --help for usage information"
        exit 1
        ;;
esac