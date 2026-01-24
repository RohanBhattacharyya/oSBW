#include "StarJsonPatch.hpp"
#include "StarJsonPath.hpp"
#include "StarLexicalCast.hpp"
#include "StarLogging.hpp"

namespace Star {

// Sentinel value returned when a test operation fails in non-throwing mode.
// Callers should check result.isNull() to detect test failure.
static Json TestFailedSentinel = Json();

Json jsonPatch(Json const& base, JsonArray const& patch) {
  auto res = base;
  try {
    for (auto const& operation : patch) {
      res = JsonPatching::applyOperation(res, operation);
      // Check for test failure sentinel
      if (res.isNull() && !base.isNull())
        return base; // Test failed, return original unchanged
    }
    return res;
  } catch (JsonException const& e) {
    throw JsonPatchException(strf("Could not apply patch to base. {}", e.what()), false);
  }
}


// Returns 0 if not found, index + 1 if found.
size_t findJsonMatch(Json const& searchable, Json const& value, JsonPath::Pointer& pointer) {
  if (searchable.isType(Json::Type::Array)) {
    auto array = searchable.toArray();
    for (size_t i = 0; i != array.size(); ++i) {
      if (jsonPartialMatch(array[i], value))
        return i + 1;
    }
  } else {
    throw JsonPatchException(strf("Search operation failure, value at '{}' is not an array.", pointer.path()), false);
  }
  return 0;
}


namespace JsonPatching {

  static const StringMap<std::function<Json(Json, Json)>> functionMap = StringMap<std::function<Json(Json, Json)>>{
      {"test", std::bind(applyTestOperation, _1, _2)},
      {"remove", std::bind(applyRemoveOperation, _1, _2)},
      {"add", std::bind(applyAddOperation, _1, _2)},
      {"replace", std::bind(applyReplaceOperation, _1, _2)},
      {"move", std::bind(applyMoveOperation, _1, _2)},
      {"copy", std::bind(applyCopyOperation, _1, _2)},
      {"merge", std::bind(applyMergeOperation, _1, _2)},
  };

  Json applyOperation(Json const& base, Json const& op, Maybe<Json> const&) {
    try {
      auto operation = op.getString("op");
      return JsonPatching::functionMap.get(operation)(base, op);
    } catch (JsonPath::TraversalException const& e) {
      // Catch TraversalException before JsonException since it's a subclass.
      // This happens when a patch operation references a path that doesn't exist.
      throw JsonPatchException(strf("Path traversal error in operation: {}", e.what()), false);
    } catch (JsonException const& e) {
      throw JsonPatchException(strf("Could not apply operation to base. {}", e.what()), false);
    } catch (MapException const&) {
      throw JsonPatchException(strf("Invalid operation: {}", op.getString("op")), false);
    }
  }

  Json applyTestOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto pointer = JsonPath::Pointer(path);
    auto inverseTest = op.getBool("inverse", false);

    // Helper to handle test failure - logs and returns sentinel instead of throwing
    // This is needed because Emscripten doesn't reliably catch C++ exceptions
    auto testFailed = [&](String const& reason) -> Json {
      Logger::debug("Patch test failure: {}", reason);
      return TestFailedSentinel;
    };

    if (op.contains("search")) {
      auto maybeSearchable = pointer.tryGet(base);
      if (!maybeSearchable) {
        if (inverseTest)
          return base;
        return testFailed(strf("path '{}' does not exist", path));
      }
      auto searchable = *maybeSearchable;
      auto searchValue = op.get("search");
      bool found = findJsonMatch(searchable, searchValue, pointer);
      if (found && inverseTest)
        return testFailed(strf("expected {} to be missing", searchValue));
      else if (!found && !inverseTest)
        return testFailed(strf("could not find {}", searchValue));
      return base;
    } else {
      auto value = op.opt("value");
      auto maybeTestValue = pointer.tryGet(base);
      if (!maybeTestValue) {
        if (inverseTest)
          return base;
        return testFailed(strf("path '{}' does not exist", path));
      }
      auto testValue = *maybeTestValue;
      if (!value) {
        if (inverseTest)
          return testFailed(strf("expected {} to be missing", path));
        return base;
      }

      if ((value && (testValue == *value)) ^ inverseTest)
        return base;
      else
        return testFailed(strf("expected {} found {}", value, testValue));
    }
  }

  Json applyRemoveOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto pointer = JsonPath::Pointer(path);

    if (op.contains("search")) {
      auto maybeSearchable = pointer.tryGet(base);
      if (!maybeSearchable)
        return base; // Path doesn't exist, nothing to remove
      auto searchable = *maybeSearchable;
      auto searchValue = op.get("search");
      if (size_t index = findJsonMatch(searchable, searchValue, pointer)) {
        auto maybeRemoved = pointer.tryRemove(base);
        if (!maybeRemoved)
          return base;
        auto maybeResult = pointer.tryAdd(*maybeRemoved, searchable.eraseIndex(index - 1));
        return maybeResult.value(base);
      }
      else
        return base;
    } else {
      // Check if path exists before trying to remove
      if (!pointer.tryGet(base))
        return base;
      auto maybeResult = pointer.tryRemove(base);
      return maybeResult.value(base);
    }
  }

  Json applyAddOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto value = op.get("value");
    auto pointer = JsonPath::Pointer(path);

    if (op.contains("search")) {
      auto maybeSearchable = pointer.tryGet(base);
      if (!maybeSearchable)
        return base; // Search path doesn't exist, skip
      auto searchable = *maybeSearchable;
      auto searchValue = op.get("search");
      if (size_t index = findJsonMatch(searchable, searchValue, pointer)) {
        auto maybeRemoved = pointer.tryRemove(base);
        if (!maybeRemoved)
          return base;
        auto maybeResult = pointer.tryAdd(*maybeRemoved, searchable.insert(index - 1, value));
        return maybeResult.value(base);
      }
      else
        return base;
    } else {
      auto maybeResult = pointer.tryAdd(base, value);
      return maybeResult.value(base);
    }
  }

  Json applyReplaceOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto value = op.get("value");
    auto pointer = JsonPath::Pointer(op.getString("path"));

    if (op.contains("search")) {
      auto maybeSearchable = pointer.tryGet(base);
      if (!maybeSearchable)
        return base; // Search path doesn't exist, skip
      auto searchable = *maybeSearchable;
      auto searchValue = op.get("search");
      if (size_t index = findJsonMatch(searchable, searchValue, pointer)) {
        auto maybeRemoved = pointer.tryRemove(base);
        if (!maybeRemoved)
          return base;
        auto maybeResult = pointer.tryAdd(*maybeRemoved, searchable.set(index - 1, value));
        return maybeResult.value(base);
      }
      else
        return base;
    } else {
      // Check if path exists before trying to replace
      if (!pointer.tryGet(base))
        return base;
      auto maybeRemoved = pointer.tryRemove(base);
      if (!maybeRemoved)
        return base;
      auto maybeResult = pointer.tryAdd(*maybeRemoved, value);
      return maybeResult.value(base);
    }
  }

  Json applyMoveOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto toPointer = JsonPath::Pointer(path);
    auto fromPointer = JsonPath::Pointer(op.getString("from"));

    if (op.contains("search")) {
      auto maybeSearchable = fromPointer.tryGet(base);
      if (!maybeSearchable)
        return base; // Source path doesn't exist, skip
      auto searchable = *maybeSearchable;
      auto searchValue = op.get("search");
      if (size_t index = findJsonMatch(searchable, searchValue, fromPointer)) {
        auto maybeResult = toPointer.tryAdd(base, searchable.get(index - 1));
        if (!maybeResult)
          return base;
        auto maybeFinal = fromPointer.tryAdd(*maybeResult, searchable.eraseIndex(index - 1));
        return maybeFinal.value(base);
      }
      else
        return base;
    } else {
      auto maybeValue = fromPointer.tryGet(base);
      if (!maybeValue)
        return base; // Source path doesn't exist, skip
      auto maybeRemoved = fromPointer.tryRemove(base);
      if (!maybeRemoved)
        return base;
      auto maybeResult = toPointer.tryAdd(*maybeRemoved, *maybeValue);
      return maybeResult.value(base);
    }
  }

  Json applyCopyOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto toPointer = JsonPath::Pointer(path);
    auto fromPointer = JsonPath::Pointer(op.getString("from"));

    if (op.contains("search")) {
      auto maybeSearchable = fromPointer.tryGet(base);
      if (!maybeSearchable)
        return base; // Source path doesn't exist, skip
      auto searchable = *maybeSearchable;
      auto searchValue = op.get("search");
      if (size_t index = findJsonMatch(searchable, searchValue, fromPointer)) {
        auto maybeResult = toPointer.tryAdd(base, searchable.get(index - 1));
        return maybeResult.value(base);
      }
      else
        return base;
    } else {
      auto maybeValue = fromPointer.tryGet(base);
      if (!maybeValue)
        return base; // Source path doesn't exist, skip
      auto maybeResult = toPointer.tryAdd(base, *maybeValue);
      return maybeResult.value(base);
    }
  }

  Json applyMergeOperation(Json const& base, Json const& op) {
    String path = op.getString("path");
    auto pointer = JsonPath::Pointer(path);

    if (op.contains("search")) {
      auto maybeSearchable = pointer.tryGet(base);
      if (!maybeSearchable)
        return base; // Search path doesn't exist, skip
      auto searchable = *maybeSearchable;
      auto searchValue = op.get("search");
      if (size_t index = findJsonMatch(searchable, searchValue, pointer)) {
        auto maybeRemoved = pointer.tryRemove(base);
        if (!maybeRemoved)
          return base;
        auto maybeResult = pointer.tryAdd(*maybeRemoved, searchable.set(index - 1, jsonMerge(searchable.get(index - 1), op.get("value"))));
        return maybeResult.value(base);
      }
      else
        return base;
    } else {
      auto maybeExisting = pointer.tryGet(base);
      if (!maybeExisting)
        return base; // Path doesn't exist, skip merge
      auto maybeRemoved = pointer.tryRemove(base);
      if (!maybeRemoved)
        return base;
      auto maybeResult = pointer.tryAdd(*maybeRemoved, jsonMerge(*maybeExisting, op.get("value")));
      return maybeResult.value(base);
    }
  }
}

}
