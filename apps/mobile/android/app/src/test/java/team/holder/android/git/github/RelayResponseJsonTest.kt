package team.holder.android.git.github

import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class RelayResponseJsonTest {
    @Test
    fun decodesJsonEscapesAndAcceptsIntegerBoundaries() {
        assertEquals(
            mapOf("text" to "\"\\/\b\u000c\n\r\tA\u263a", "zero" to 0, "max" to Int.MAX_VALUE),
            parseRelayResponseObject(""" {"text":"\"\\\/\b\f\n\r\t\u0041\u263a","zero":0,"max":2147483647} """),
        )
        val random = java.util.Random(197)
        repeat(100) {
            val text = buildString { repeat(128) { append(random.nextInt(0x10000).toChar()) } }
            assertEquals(mapOf("text" to text), parseRelayResponseObject(JSONObject().put("text", text).toString()))
        }
    }

    @Test
    fun rejectsTruncationInvalidEscapesAndUnsupportedValues() {
        val valid = """{"text":"value\n\u0041","number":123}"""
        for (length in 0 until valid.length) assertNull(parseRelayResponseObject(valid.take(length)))
        for (value in listOf(
            "-1", "-0", "+1", "01", "1.0", "1e2", "2147483648", "true", "null", "[]", "{}",
            "\"\\x\"", "\"\\u00xz\"", "\"raw\nnewline\"", "\"raw\u0000null\"",
        )) {
            assertNull(value, parseRelayResponseObject("{\"field\":$value}"))
        }
        assertNull(parseRelayResponseObject("{\"field\":" + "[".repeat(8000)))
        assertNull(parseRelayResponseObject("{\"field\":1}\u0000"))
    }

    @Test
    fun rejectsDuplicateDecodedKeysExtraFieldsAndTrailingData() {
        assertNull(parseRelayResponseObject("""{"error":"unexpected","err\u006fr":"unexpected"}"""))
        assertNull(parseRelayResponseObject("""{"a":0,"b":0,"c":0,"d":0,"e":0,"f":0}"""))
        assertNull(parseRelayResponseObject("""{"a":0,}"""))
        assertNull(parseRelayResponseObject("""{"a":0} {}"""))
        assertEquals(emptyMap<String, Any>(), parseRelayResponseObject("{}"))
    }
}
