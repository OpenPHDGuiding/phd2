---
type: "manual"
---

Build the entire PHD2 project from scratch and systematically fix all compilation errors, linking issues, and dependency problems that are encountered during the build process. 

Specifically:
1. First, analyze the project structure and identify the build system being used (CMake, Make, etc.)
2. Check for and install any missing dependencies or build tools required
3. Attempt a clean build of the entire project
4. For each error encountered during compilation:
   - Identify the root cause of the error
   - Research the appropriate fix within the codebase context
   - Apply the necessary code changes, dependency installations, or configuration updates
   - Verify the fix resolves the specific error without introducing new issues
5. Continue this process iteratively until the project builds successfully without errors
6. Provide a summary of all issues found and how they were resolved
7. Suggest running tests after successful compilation to ensure functionality is preserved

The goal is to achieve a complete, error-free build of the PHD2 project that can be used for development and testing.