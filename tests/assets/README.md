# Test Assets

This directory contains test assets used by the disasterparty test suite for file upload and attachment testing.

## Files

### Images
- `test_image_small.jpg` - Small JPEG image (9KB) from Wikimedia Commons
- `test_image_medium.png` - Medium PNG image (2KB) from Wikimedia Commons

### Text Files
- `test_text_small.txt` - Small text file for basic text upload tests
- `test_text_large.txt` - Larger text file with Lorem ipsum content
- `test_empty.txt` - Empty text file (0 bytes)
- `test_zero_byte.dat` - Zero-byte binary file

### Data Files
- `test_data.json` - Sample JSON data file
- `test_data.csv` - Sample CSV data file
- `test_document.pdf` - Sample PDF document (13KB) from W3C test resources

### Binary Files
- `test_binary.bin` - Random binary data (1KB) for unsupported format testing
- `test_large_file.dat` - Large file (5MB) for large file upload testing

## Licensing

All content in this directory is either:
- Public domain content
- Content from Wikimedia Commons under open licenses
- Generated test data in the public domain

These files are intended solely for testing purposes and should not be used in production.

## Usage

These assets are used by various test files including:
- `test_file_attachments_dp.c`
- `test_upload_large_file_dp.c`
- `test_upload_zero_byte_file_dp.c`
- `test_unsupported_file_uploads_dp.c`
- And other file-related tests

Tests should reference these files using relative paths from the tests directory:
```c
const char* test_file = "assets/test_image_small.jpg";
```