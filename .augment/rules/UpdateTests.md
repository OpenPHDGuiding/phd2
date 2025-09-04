---
type: "manual"
---

I will provide you with a directory/folder path. I need you to generate comprehensive unit tests for all modules within that directory. The requirements are:

1. **Scope**: Create unit tests for every module/file in the specified directory
2. **Coverage**: Tests should cover all functions, methods, classes, and edge cases within each module
3. **Test Structure**: 
   - Place all test files in a `@tests` directory 
   - Mirror the original directory structure within `@tests` (e.g., if original file is `src/utils/helper.py`, test should be `@tests/src/utils/test_helper.py`)
   - Follow standard naming conventions (prefix test files with `test_` or suffix with `_test`)
4. **Test Quality**:
   - Include positive test cases (normal functionality)
   - Include negative test cases (error handling, invalid inputs)
   - Include edge cases and boundary conditions
   - Use appropriate test frameworks for the language (e.g., pytest for Python, Jest for JavaScript, etc.)
   - Include setup/teardown methods where needed
   - Add descriptive test names and docstrings/comments explaining what each test validates

5. **Dependencies**: Handle any external dependencies through mocking or test doubles where appropriate

Please analyze the codebase first to understand the structure, then create a comprehensive test suite that ensures high code coverage and reliability.