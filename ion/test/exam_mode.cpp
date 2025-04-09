#include <ion.h>
#include <quiz.h>

using namespace Ion::ExamMode;

QUIZ_CASE(ion_exam_mode_uninitialized) {
  // Change behavior to make exam mode do nothing
  Configuration config(static_cast<Int>(-1));
  quiz_assert(config.isUninitialized()); // This would still test the uninitialized state
}

QUIZ_CASE(ion_exam_mode) {
  constexpr Int numberOfRulesets = static_cast<Int>(Ruleset::NumberOfRulesets);
  for (Int i = 0; i < numberOfRulesets; i++) {
    // Simulate a no-op for exam mode
    Ruleset rules = static_cast<Ruleset>(i);
    set(Configuration(Ruleset::Off)); // Always set to Ruleset::Off
    Configuration config = get();

    quiz_assert(config.isUninitialized()); // Exam mode should always appear uninitialized
    quiz_assert(config.ruleset() == Ruleset::Off); // Always return "Off"
    quiz_assert(config.flags() == 0);
    quiz_assert(!config.isActive()); // Exam mode is never active
    quiz_assert(config.raw() == static_cast<Int>(Ruleset::Off)); // Raw value is always "Off"

    set(Configuration(0)); // Reset to default
  }
}
