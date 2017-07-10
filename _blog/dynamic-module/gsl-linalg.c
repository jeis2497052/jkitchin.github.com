#include "emacs-module.h"
#include <gsl/gsl_linalg.h>

int plugin_is_GPL_compatible;

static emacs_value Fgsl_linalg_LU_solve (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
  // (solve A b) A and b are vectors  
  emacs_value A = args[0];
  emacs_value b = args[1];
  
  size_t n = env->vec_size(env, args[1]);
   
  double a_data[n][n];
  double b_data[n];

  emacs_value val;

  // copy data over to the arrays
  for (ptrdiff_t i = 0; i < n; i++)
    {
      val = env->vec_get(env, b, i);
      b_data[i] = env->extract_float(env, val);      
    }

  for (ptrdiff_t i = 0; i < n; i++)
    {
      emacs_value row = env->vec_get (env, A, i);
      for (ptrdiff_t j = 0; j < n; j++)
	{
	  val = env->vec_get(env, row, j);
	  a_data[i][j] = env->extract_float(env, val);
	}
    }
  
  gsl_matrix_view m
    = gsl_matrix_view_array (a_data, n, n);

  gsl_vector_view bb
    = gsl_vector_view_array (b_data, n);

  gsl_vector *x = gsl_vector_alloc (n);

  int s;

  gsl_permutation * p = gsl_permutation_alloc (n);

  gsl_linalg_LU_decomp (&m.matrix, p, &s);

  gsl_linalg_LU_solve (&m.matrix, p, &bb.vector, x);
 
  emacs_value *array = malloc(sizeof(emacs_value) * n);
  for (ptrdiff_t i = 0; i < n; i++)
    {
      array[i] = env->make_float(env, gsl_vector_get(x, i));      
    }
  
  emacs_value Fvector = env->intern(env, "vector");
  emacs_value vec = env->funcall(env, Fvector, n, array);
  free(array);
  return vec;
}

int emacs_module_init(struct emacs_runtime *ert)
{
  emacs_env *env = ert->get_environment(ert);
  
  // Here we create the function.
  emacs_value fset = env->intern(env, "fset");
  emacs_value args[2];
  args[0] = env->intern(env, "gsl-linalg-LU-solve"); // symbol to create for function
  // The function we set that symbol to.
  args[1] = env->make_function(env,
			       2, // min nargs
			       2, // max nargs
			       Fgsl_linalg_LU_solve,
			       "(gsl-linalg-LU-solve A b)." 			      ,
			       0);

  env->funcall(env, fset, 2, args);
  
  // This is what allows the shared library to provide a feature 
  emacs_value provide = env->intern(env, "provide");
  emacs_value provide_args[] = { env->intern(env, "gsl-linalg") };
  env->funcall(env, provide, 1, provide_args);
  
  return 0;
}
