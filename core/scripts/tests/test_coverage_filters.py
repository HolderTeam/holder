"""Reporting filters must never conceal real function or source-line misses."""

import importlib.util
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("coverage_filters", Path(__file__).parents[1] / "coverage_filters.py")
coverage_filters = importlib.util.module_from_spec(spec)
spec.loader.exec_module(coverage_filters)

ALIAS = "_ZN12_GLOBAL__N_126CTimeZoneProviderCallbacksD0Ev"
BODY = ALIAS.replace("D0Ev", "D2Ev")


class CoverageFilterTests(unittest.TestCase):
    def modern(self, *, source="/repo/core/src/api/HostCApi.cpp", alias=ALIAS, alias_hits=0, body_hits=4):
        body = alias.replace("D0Ev", "D2Ev")
        return (f"TN:\nSF:{source}\nFNL:0,23,23\nFNA:0,{alias_hits},{alias}\n"
                f"FNA:0,{body_hits},{body}\nFNL:1,25,30\nFNA:1,0,real_missing_function\n"
                "FNF:2\nFNH:1\nDA:23,4\nDA:25,0\nBRDA:25,0,0,0\nLF:2\nLH:1\nend_of_record\n")

    def test_modern_alias_filter_preserves_body_real_miss_and_all_line_branch_data(self):
        original = self.modern()
        filtered, count = coverage_filters.filter_lcov(original)
        self.assertEqual(count, 1)
        self.assertNotIn(ALIAS, filtered)
        self.assertIn(f"FNA:0,4,{BODY}\n", filtered)
        self.assertIn("FNA:1,0,real_missing_function\n", filtered)
        for prefix in ("DA:", "BRDA:", "LF:", "LH:", "FNL:", "FNF:", "FNH:"):
            self.assertEqual([line for line in original.splitlines() if line.startswith(prefix)],
                             [line for line in filtered.splitlines() if line.startswith(prefix)])
        self.assertEqual(coverage_filters.filter_lcov(filtered), (filtered, 0))

    def test_legacy_format_preserves_covered_body_and_source_lines(self):
        original = (f"SF:src/api/HostCApi.cpp\nFN:23,{ALIAS}\nFN:23,{BODY}\n"
                    f"FNDA:0,{ALIAS}\nFNDA:4,{BODY}\nFNF:2\nFNH:1\nDA:23,4\nend_of_record\n")
        filtered, count = coverage_filters.filter_lcov(original)
        self.assertEqual(count, 1)
        self.assertNotIn(ALIAS, filtered)
        self.assertIn(f"FN:23,{BODY}\n", filtered)
        self.assertIn(f"FNDA:4,{BODY}\n", filtered)
        self.assertIn("FNF:1\nFNH:1\nDA:23,4\n", filtered)

    def test_no_exclusion_without_a_covered_matching_body(self):
        for kwargs in ({"body_hits": 0}, {"alias_hits": 1},
                       {"source": "/repo/core/src/unrelated.cpp"},
                       {"alias": "_ZN6CustomD0Ev"}):
            with self.subTest(kwargs=kwargs):
                original = self.modern(**kwargs)
                self.assertEqual(coverage_filters.filter_lcov(original), (original, 0))

    def test_matching_body_in_another_record_is_not_evidence_for_exclusion(self):
        untested = self.modern(body_hits=0)
        covered_elsewhere = self.modern(source="/different/translation/unit.cpp")
        text = untested + covered_elsewhere
        self.assertEqual(coverage_filters.filter_lcov(text), (text, 0))

    def test_other_zero_hit_aliases_in_the_same_file_are_retained(self):
        original = self.modern().replace("FNF:2", "FNL:2,40,40\nFNA:2,0,OtherD0Ev\nFNA:2,1,OtherD2Ev\nFNF:3")
        filtered, count = coverage_filters.filter_lcov(original)
        self.assertEqual(count, 1)
        self.assertIn("FNA:2,0,OtherD0Ev\n", filtered)
        self.assertIn("FNF:3\nFNH:2\n", filtered)

    def test_covered_lambda_entry_normalization_preserves_function_body_and_branches(self):
        symbol = next(iter(coverage_filters.COVERED_ENTRY_LINES["src/api/ProjectMaintenance.cpp"]))
        original = (f"SF:/repo/core/src/api/ProjectMaintenance.cpp\nFNL:0,42,45\nFNA:0,8,{symbol}\n"
                    "FNF:1\nFNH:1\nDA:42,0\nDA:43,0\nDA:44,8\nBRDA:43,0,0,0\nLF:3\nLH:1\nend_of_record\n")
        expected = original.replace("DA:42,0", "DA:42,8").replace("LH:1", "LH:2")
        self.assertEqual(coverage_filters.filter_lcov(original), (expected, 0))
        self.assertEqual(coverage_filters.filter_lcov(expected), (expected, 0))

    def test_lambda_entry_requires_exact_covered_function_in_the_same_file(self):
        symbol = next(iter(coverage_filters.COVERED_ENTRY_LINES["src/api/ProjectMaintenance.cpp"]))
        original = (f"SF:src/api/ProjectMaintenance.cpp\nFNL:0,42,45\nFNA:0,8,{symbol}\n"
                    "DA:42,0\nLF:1\nLH:0\nend_of_record\n")
        for text in (original.replace("FNA:0,8,", "FNA:0,0,"),
                     original.replace("ProjectMaintenance.cpp", "Other.cpp"),
                     original.replace(symbol, "other_lambda"),
                     original.replace("FNL:0,42,45\n", ""),
                     original.replace("DA:42,0", "DA:42,3")):
            self.assertEqual(coverage_filters.filter_lcov(text), (text, 0))

    def test_legacy_lambda_entry_uses_the_recorded_positive_count(self):
        symbol = next(iter(coverage_filters.COVERED_ENTRY_LINES["src/api/ProjectMaintenance.cpp"]))
        original = (f"SF:src/api/ProjectMaintenance.cpp\nFN:42,{symbol}\nFNDA:8,{symbol}\n"
                    "FNF:1\nFNH:1\nDA:42,0,checksum\nLF:1\nLH:0\nend_of_record\n")
        expected = original.replace("DA:42,0,checksum", "DA:42,8,checksum").replace("LH:0", "LH:1")
        self.assertEqual(coverage_filters.filter_lcov(original), (expected, 0))


if __name__ == "__main__":
    unittest.main()
