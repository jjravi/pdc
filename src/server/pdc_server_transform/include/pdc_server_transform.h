//  #include "rpc_engine.h"
//  
//  #ifndef MY_RPC_H
//  #define MY_RPC_H
//  
//  /** @struct my_rpc_in_t
//   * My super secret structure you can't access fields
//   */
//  
//  /**
//   * @struct my_rpc_in_t
//   * @brief This is a struct
//   *
//   * Typical usage requires that we either supply the number of times to bar followed by some other baz ...
//   * @code
//   * int nbars = 12; // must be uint8_t because of some reason
//   * @endcode
//   *
//   *  @var my_rpc_in_t::int32_t
//   *    A foo.
//   *  @var my_rpc_in_t::hg_bulk_t bulk_handle
//   *    Also a Foo.
//   *  @var my_rpc_in_t::baz
//   *    (unused field)
//   */
//  MERCURY_GEN_PROC(my_rpc_in_t, ((int32_t)(input_val))((hg_bulk_t)(bulk_handle)))
//  
//  /// @brief ret
//  /// Typical usage requires that we either supply the number of times to bar followed by some other baz ...
//  /// @code
//  /// int nbars = 12; // must be uint8_t because of some reason
//  /// @endcode
//  MERCURY_GEN_PROC(my_rpc_out_t, ((int32_t)(ret)))
//  
//  hg_id_t my_rpc_register();
//  
//  #endif // MY_RPC_H
