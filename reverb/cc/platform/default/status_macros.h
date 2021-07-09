// Copyright 2019 DeepMind Technologies Limited.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef REVERB_CC_PLATFORM_DEFAULT_STATUS_H_
#define REVERB_CC_PLATFORM_DEFAULT_STATUS_H_

#include "absl/status/status.h"

#ifdef __has_builtin
#define REVERB_HAS_BUILTIN(x) __has_builtin(x)
#else
#define REVERB_HAS_BUILTIN(x) 0
#endif

// Compilers can be told that a certain branch is not likely to be taken
// (for instance, a CHECK failure), and use that information in static
// analysis. Giving it this information can help it optimize for the
// common case in the absence of better information (ie.
// -fprofile-arcs).
#if REVERB_HAS_BUILTIN(__builtin_expect) || (defined(__GNUC__) && __GNUC__ >= 3)
#define REVERB_PREDICT_FALSE(x) (__builtin_expect(x, 0))
#define REVERB_PREDICT_TRUE(x) (__builtin_expect(!!(x), 1))
#else
#define REVERB_PREDICT_FALSE(x) (x)
#define REVERB_PREDICT_TRUE(x) (x)
#endif

// Evaluates an expression that produces a `absl::Status`. If the status
// is not ok, returns it from the current function.
//
// For example:
//   absl::Status MultiStepFunction() {
//     REVERB_RETURN_IF_ERROR(Function(args...));
//     REVERB_RETURN_IF_ERROR(foo.Method(args...));
//     return absl::OkStatus();
//   }
//
// The macro ends with an `internal::StatusBuilder` which allows the returned
// status to be extended with more details.  Any chained expressions after the
// macro will not be evaluated unless there is an error.
//
// For example:
//   absl::Status MultiStepFunction() {
//     REVERB_RETURN_IF_ERROR(Function(args...)) << "in MultiStepFunction";
//     REVERB_RETURN_IF_ERROR(foo.Method(args...)).Log(base_logging::ERROR)
//         << "while processing query: " << query.DebugString();
//     return absl::OkStatus();
//   }
//
// `internal::StatusBuilder` supports adapting the builder chain using a
// `With` method and a functor.  This allows for powerful extensions to the
// macro.
//
// For example, teams can define local policies to use across their code:
//
//   StatusBuilder TeamPolicy(StatusBuilder builder) {
//     return std::move(builder.Log(base_logging::WARNING).Attach(...));
//   }
//
//   REVERB_RETURN_IF_ERROR(foo()).With(TeamPolicy);
//   REVERB_RETURN_IF_ERROR(bar()).With(TeamPolicy);
//
// If using this macro inside a lambda, you need to annotate the return type
// to avoid confusion between an `internal::StatusBuilder` and a
// `absl::Status` type. E.g.
//
//   []() -> absl::Status {
//     REVERB_RETURN_IF_ERROR(Function(args...));
//     REVERB_RETURN_IF_ERROR(foo.Method(args...));
//     return absl::OkStatus();
//   }
#define REVERB_RETURN_IF_ERROR(...)                      \
  do {                                                   \
    ::absl::Status _status = (__VA_ARGS__);                  \
    if (REVERB_PREDICT_FALSE(!_status.ok())) return _status; \
  } while (0)


// Executes an expression `rexpr` that returns a `absl::StatusOr<T>`. On
// OK, extracts its value into the variable defined by `lhs`, otherwise returns
// from the current function. By default the error status is returned
// unchanged, but it may be modified by an `error_expression`. If there is an
// error, `lhs` is not evaluated; thus any side effects that `lhs` may have
// only occur in the success case.
//
// Interface:
//
//   REVERB_ASSIGN_OR_RETURN(lhs, rexpr)
//   REVERB_ASSIGN_OR_RETURN(lhs, rexpr, error_expression);
//
// WARNING: expands into multiple statements; it cannot be used in a single
// statement (e.g. as the body of an if statement without {})!
//
// Example: Declaring and initializing a new variable (ValueType can be anything
//          that can be initialized with assignment, including references):
//   REVERB_ASSIGN_OR_RETURN(ValueType value, MaybeGetValue(arg));
//
// Example: Assigning to an existing variable:
//   ValueType value;
//   REVERB_ASSIGN_OR_RETURN(value, MaybeGetValue(arg));
//
// Example: Assigning to an expression with side effects:
//   MyProto data;
//   REVERB_ASSIGN_OR_RETURN(*data.mutable_str(), MaybeGetValue(arg));
//   // No field "str" is added on error.
//
// Example: Assigning to a std::unique_ptr.
//   REVERB_ASSIGN_OR_RETURN(std::unique_ptr<T> ptr, MaybeGetPtr(arg));
//
// If passed, the `error_expression` is evaluated to produce the return
// value. The expression may reference any variable visible in scope, as
// well as an `internal::StatusBuilder` object populated with the error and
// named by a single underscore `_`. The expression typically uses the
// builder to modify the status and is returned directly in manner similar
// to REVERB_RETURN_IF_ERROR. The expression may, however, evaluate to any type
// returnable by the function, including (void). For example:
//
// Example: Adjusting the error message.
//   REVERB_ASSIGN_OR_RETURN(ValueType value, MaybeGetValue(query),
//                    _ << "while processing query " << query.DebugString());
//
// Example: Logging the error on failure.
//   REVERB_ASSIGN_OR_RETURN(
//.      ValueType value, MaybeGetValue(query), _.LogError());
//
#define REVERB_ASSIGN_OR_RETURN_IMPL(statusor, lhs, rexpr) \
  auto statusor = (rexpr);                             \
  if (REVERB_PREDICT_FALSE(!statusor.ok())) {              \
    return statusor.status();                          \
  }                                                    \
  lhs = std::move(statusor.ValueOrDie())


#define REVERB_CHECK_OK(val) CHECK_EQ(::absl::OkStatus(), (val))

#endif  // REVERB_CC_PLATFORM_DEFAULT_STATUS_H_
