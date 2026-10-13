package team.holder.android.git.github

/** The relay's closed schema permits only a flat object of strings and nonnegative Ints.
 * Reject all other JSON values, duplicate decoded keys, and non-JSON syntax. Parsing is
 * iterative with a monotonically advancing cursor: no regex engine, recursion or reparsing.
 * The HTTP reader bounds bytes; the length check also bounds direct parser callers. */
internal fun parseRelayResponseObject(input: String): Map<String, Any>? =
    if (input.length > MAX_RELAY_RESPONSE_BYTES) null else RelayResponseJson(input).parse()

private class RelayResponseJson(private val input: String) {
    private var position = 0

    fun parse(): Map<String, Any>? {
        skipSpace()
        if (!take('{')) return null
        skipSpace()
        val fields = mutableMapOf<String, Any>()
        if (!take('}')) {
            while (true) {
                if (fields.size == 5) return null
                val key = readString() ?: return null
                if (fields.containsKey(key)) return null
                skipSpace()
                if (!take(':')) return null
                skipSpace()
                val value = if (peek() == '"') readString() else readInt()
                fields[key] = value ?: return null
                skipSpace()
                if (take('}')) break
                if (!take(',')) return null
                skipSpace()
            }
        }
        skipSpace()
        return fields.takeIf { position == input.length }
    }

    private fun readString(): String? {
        if (!take('"')) return null
        val result = StringBuilder()
        while (position < input.length) {
            val char = input[position++]
            when {
                char == '"' -> return result.toString()
                char < ' ' -> return null
                char == '\\' -> {
                    if (position == input.length) return null
                    val escaped = when (val escape = input[position++]) {
                        '"', '\\', '/' -> escape
                        'b' -> '\b'
                        'f' -> '\u000c'
                        'n' -> '\n'
                        'r' -> '\r'
                        't' -> '\t'
                        'u' -> readHexChar() ?: return null
                        else -> return null
                    }
                    result.append(escaped)
                }
                else -> result.append(char)
            }
        }
        return null
    }

    private fun readHexChar(): Char? {
        if (input.length - position < 4) return null
        var value = 0
        repeat(4) {
            val digit = when (val char = input[position++]) {
                in '0'..'9' -> char - '0'
                in 'a'..'f' -> char - 'a' + 10
                in 'A'..'F' -> char - 'A' + 10
                else -> return null
            }
            value = value * 16 + digit
        }
        return value.toChar()
    }

    private fun readInt(): Int? {
        if (take('0')) return 0 // A following digit will fail the delimiter check.
        if (peek() !in '1'..'9') return null
        var value = 0
        while (peek() in '0'..'9') {
            val digit = input[position++] - '0'
            if (value > (Int.MAX_VALUE - digit) / 10) return null
            value = value * 10 + digit
        }
        return value
    }

    private fun skipSpace() {
        while (peek() in " \t\r\n") position++
    }

    private fun peek(): Char = input.getOrNull(position) ?: '\u0000'

    private fun take(char: Char): Boolean {
        if (peek() != char) return false
        position++
        return true
    }
}
