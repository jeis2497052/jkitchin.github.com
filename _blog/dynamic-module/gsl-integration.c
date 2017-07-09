#include <gsl/gsl_integration.h>
#include "emacs-module.h"

int plugin_is_GPL_compatible;

static emacs_value F_gsl_integrate (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
  // nested function - only supported as an extension in gcc
  double f (double x, void *params) 
  {
    emacs_value fn = args[0];  // function we will integrate
    emacs_value x2[] = { env->make_float(env, x), params };
    emacs_value y = env->funcall(env, fn, 2, &x2);   
    
    return env->extract_float (env, y);
  }

  double a = env->extract_float (env, args[1]);
  double b = env->extract_float (env, args[2]);

  // default values for optional arguments
  double epsabs = 0.0;
  double epsrel = 1e-7;
  size_t limit = 1000;
  double result, error; 

  // Here is how I handle the optional arguments
  // (gsl-integrate func a b params epsabs epsrel limit)
  gsl_function F;
  F.function = &f;
  if (nargs >= 4) {F.params = args[3];}
  if (nargs >= 5 && env->is_not_nil(env, args[4])) {epsabs = env->extract_float(env, args[4]);}
  if (nargs >= 6 && env->is_not_nil(env, args[5])) {epsrel = env->extract_float(env, args[5]);}
  if (nargs >= 7 && env->is_not_nil(env, args[6])) {limit = env->extract_integer(env, args[6]);}

  gsl_integration_workspace * w = gsl_integration_workspace_alloc (limit);

  gsl_integration_qags (&F, // gsl_function pointer
			a, // lower integration bound
			b, // upper integration bound
			epsabs, // absolute error tolerance
			epsrel, // relative error tolerance
			limit, // max number of subintervals for integration
                        w, // the workspace
			// pointers to put results and error in
			&result, &error);

  gsl_integration_workspace_free (w);
    
  // make a list of (result error) to return
  emacs_value Qlist = env->intern(env, "list");
  emacs_value Qresult = env->make_float (env, result);
  emacs_value Qerror = env->make_float (env, error);
  emacs_value list_args[] = { Qresult, Qerror };
  return env->funcall(env, Qlist, 2, list_args);
}

int emacs_module_init(struct emacs_runtime *ert)
{
  emacs_env *env = ert->get_environment(ert);
  
  // Here we create the function.
  emacs_value fset = env->intern(env, "fset");
  emacs_value args[2];
  args[0] = env->intern(env, "gsl-integration-qags"); // symbol to create for function
  // The function we set that symbol to.
  args[1] = env->make_function(env,
			       3, // min nargs
			       7, // max nargs
			       F_gsl_integrate,
			       "(gsl-integration-qags F A B &optional PARAMS EPSABS EPSREL LIMIT)\n" \
			       "Integrate F(x; params) from A to B.\n" \
			       "F is a function of a single variable and parameters.\n" \
			       "A is the lower bound of integration\n"	\
			       "B is the upper bound of integration.\n" \
			       "Optional parameters:\n"\
			       "PARAMS is a list of params to pass to F.\n" \
			       "EPSABS is a float (default 0.0) and is the absolute error tolerance.\n" \
			       "EPSREL is a float (default 1e-7) and is the relative error tolerance.\n" \
			       "LIMIT is the maximum number of subintervals for the integration (default 1000).\n" \
			       "Returns (list result error-estimate).\n" \
			       "See https://www.gnu.org/software/gsl/manual/html_node/QAGS-adaptive-integration-with-singularities.html.",
			       0);
  // This is basically (fset 'gsl-integration-qags (lambda func))
  env->funcall(env, fset, 2, args);
  
  // This is what allows the shared library to provide a feature 
  emacs_value provide = env->intern(env, "provide");
  emacs_value provide_args[] = { env->intern(env, "gsl-integration") };
  env->funcall(env, provide, 1, provide_args);
  
  return 0;
}
