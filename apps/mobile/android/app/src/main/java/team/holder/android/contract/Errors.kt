// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
package team.holder.android.contract

/** The thing asked for does not exist, or is in the trash. */
class NotFoundException(message: String) : ContractException(4040, message)

/** The request conflicts with what exists: what would be created already exists, such as a card with the same ID or file, or the thing to change is not in the state the request needs, such as restoring a card that is not in the trash, or removing a tag that a card would still carry in its prose. */
class ConflictException(message: String) : ContractException(4090, message)

/** The keyring that holds an encrypted project's key cannot be used: it is locked, or this device has none. */
class KeyringUnavailableException(message: String) : ContractException(5030, message)

/** An encrypted project's key is not on this device; the project's recovery token restores it. */
class KeyMissingException(message: String) : ContractException(4091, message)

/** A project's stored data is damaged or unsafe: an encrypted file does not decrypt, or holds plaintext. */
class DataCorruptedException(message: String) : ContractException(5002, message)
