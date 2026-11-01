// Generated from contract/interface by holder_contract. Do not edit; run make -C contract update.
package team.holder.android.contract

/** An error a contract operation reports. [code] is its error code in core's C API. */
open class ContractException(val code: Int, message: String) : RuntimeException(message)

/** An argument the contract's checks reject. */
class InvalidArgumentException(message: String) : ContractException(4000, message)

/** An unexpected failure. */
class UnexpectedFailureException(message: String) : ContractException(5000, message)

/** Core could not allocate memory. */
class AllocationFailureException(message: String) : ContractException(5001, message)

/** A field of a partial update: kept as it is, cleared, or set to a value. */
sealed interface Change<out T> {
    /** Leave the field as it is. */
    object Keep : Change<Nothing>

    /** Make the field null. */
    object Clear : Change<Nothing>

    /** Set the field to [value]. */
    data class Set<out T>(val value: T) : Change<T>
}
