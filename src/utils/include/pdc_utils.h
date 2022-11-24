#ifndef PDC_UTILS_H
#define PDC_UTILS_H

#include "pdc_public.h"

/**
 * Get the size of a data type
 *
 * \param dtype[IN]             Data type
 *
 * \return Size of the data type
 */
int PDC_get_var_type_size(pdc_var_type_t dtype);

void print_symbols();

#endif /* PDC_UTILS_H */

