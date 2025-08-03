# Project Directives

## Project Documentation Structure

1. **Main project description** originally comes from `docs/PROJECT.md`
2. **Project roadmap** is maintained in `docs/ROADMAP.md`
3. **Changelog** is maintained in `NEWS.md`
4. **Documentation rollup** exists in `DOCUMENTATION.md` as a comprehensive markdown format designed for AI consumption, consolidating all manual page documentation

## Development Workflow

5. **Always commit and push** after every individual task (or subtask) is complete
6. **Keep README.md up-to-date** with current project status and information

## API and Code Organization

7. **Public API calls** go in `disasterparty.h`
8. **Private API calls** go in `dp_private.h`

## Documentation Requirements

9. **Always create brand new manual pages** for:
   - Each brand new function
   - Each brand new public structure

## Testing Requirements

10. **Testsuite integration**:
   - All new tests should be added to the testsuite
   - Tests should not be thrown away - they must be saved and committed to prevent future regressions
   - All new tests must properly integrate with the GNU autotools testsuite
   - Tests should return exit code 77 when skipped

## Release Process

11. **When cutting releases**:
    - **10a.** Ensure version consistency across:
      - `configure.ac`
      - `disasterparty.h` and/or `dp_internal.h`
      - `README.md`
      - `docs/PROJECT.md`
      - All manual pages
    - **10b.** Ensure every manual page's date is the current date