/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: irychkov <irychkov@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/05 15:43:09 by omartela          #+#    #+#             */
/*   Updated: 2024/10/15 14:48:02 by irychkov         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/minishell.h"

int is_open_quote(char *str)
{
	int i;
	int in_single_quotes;
	int in_double_quotes;

	i = 0;
	in_single_quotes = 0;
	in_double_quotes = 0;
	while (str[i])
	{
		if (str[i] == '\"' && !in_single_quotes)
			in_double_quotes = !in_double_quotes;
		if (str[i] == '\'' && !in_double_quotes)
			in_single_quotes = !in_single_quotes;
		i++;
	}
	if (in_single_quotes || in_double_quotes)
		return (1);
	return (0);
}

static void increase_shlvl(t_shell *sh)
{
	char	*temp_itoa;
	int		temp_atoi;
	char	*value;
	int		i;
	int		j;

	i = 0;
	j = 0;
	temp_atoi = 0;
	temp_itoa = NULL;
	value = NULL;

	while (sh->envp[i])
	{
		if (is_check_key_equal(sh->envp[i], "SHLVL"))
		{
			value = get_value(sh->envp[i]);
			if (value)
			{
				j = 0;
				while (value[j])
				{
					if (!ft_isdigit(value[j]))
					{
						free(value);
						if (set_variables(sh, "SHLVL", "1"))
						{
							error_sys("Setting SHLVL failed\n");
							return ;
						}
						return ;
					}
					j++;
				}
				temp_atoi = ft_atoi(value);
				if (!temp_atoi)
				{
					free(value);
					error_sys("Setting SHLVL failed\n");
					return ;
				}
				temp_atoi += 1;
				temp_itoa = ft_itoa(temp_atoi);
				if (!temp_itoa)
				{
					free(value);
					error_sys("Setting SHLVL failed\n");
					return ;
				}
				if (set_variables(sh, "SHLVL", temp_itoa))
				{
					free(value);
					free(temp_itoa);
					error_sys("Setting SHLVL failed\n");
					return ;
				}
				free(value);
				free(temp_itoa);
				return ;
			}
			else
			{
				if (set_variables(sh, "SHLVL", "1"))
				{
					error_sys("Setting SHLVL failed\n");
					return ;
				}
			}
		}
		++i;
	}
	if (append_table(&sh->envp, "SHLVL", "1"))
	{
		error_sys("Setting SHLVL failed\n");
		return ;
	}
	if (append_table(&sh->local_shellvars, "SHLVL", "1"))
	{
		error_sys("Setting SHLVL failed\n");
		return ;
	}
	sort_table(sh->local_shellvars);
	return ;
}

static void	initialize_shell(t_shell *sh, char **envp)
{
	t_heredoc	*hd;

	sh->exit_status = 0;
	sh->num_cmds = 0;
	sh->commands = NULL;
	sh->promt = NULL;
	sh->pipes = NULL;
	copy_env(envp, sh);
	if (envp)
		increase_shlvl(sh);
	sh->homepath = expand(envp, "HOME");
	if (!sh->homepath)
	{
		error_sys("ft_strdup failed for getenv\n");
		exit (1);
	}
	hd = malloc(sizeof(t_heredoc));
	if (!hd)
	{
		error_sys("malloc failed for t_heredoc\n");
		exit(1);
	}
	sh->hd = hd;
	hd->heredoc_fds = NULL;
	hd->num_heredocs = 0;
	hd->heredoc_index = 0;
	ft_memset(&sh->org_sig_int, 0, sizeof(sh->org_sig_int));
	ft_memset(&sh->org_sig_quit, 0, sizeof(sh->org_sig_quit));
	if (init_signal(sh))
		exit (1);
}

int	add_prompt(t_shell *sh, char *input)
{
	char *temp;

	if (sh->promt)
	{
		temp = ft_strjoin(sh->promt, input);
		if (!temp)
		{
			error_sys("ft_strjoin failed\n");
			return (1);
		}
		free(sh->promt);
		sh->promt = temp;
	}
	else
	{
		sh->promt = ft_strdup(input);
		if (!sh->promt)
		{
			error_sys("ft_strdup failed\n");
			return (1);
		}
	}
	return (0);
}

static void	process_input(t_shell *sh, char *input)
{
	int		len;
	char 	*split_input;
	char	*next_input;

	next_input = NULL;
	split_input = NULL;

	split_input = trim_spaces(input);
	if (check_syntax(split_input))
	{
		sh->exit_status = 2;
		return ;
	}
	input = split_and_parse(split_input, sh);
	if (!input)
	{
		error_sys("split_and_parse failed\n");
		sh->exit_status = 1;
		return ;
	}
	split_input = ft_add_spaces(input);
	free(input);
	if (!split_input)
	{
		error_sys("ft_add_spaces failed\n");
		sh->exit_status = 1;
		return ;
	}
	input = split_input;
	if (is_heredoc(input))
	{
		if (handle_here_doc(sh, input))
		{
			error_sys("handle_here_doc failed\n");
			free(input);
			return ;
		}
	}
	len = ft_strlen(input);
	while ((len > 0 && input[len - 1] == '|') || (len > 2 && input[len - 1] == '&' && input[len - 2] == '&') || (len > 0 && is_open_quote(input)))
	{
/* 		next_input = readline("> ");
		if (!next_input)
		{
			free(input);
			//printf("Exit \n");
			return ;
		} */
		//Snippet for tester
		if (isatty(fileno(stdin)))
			next_input = readline("> ");
		else
		{
			char *line;
			line = get_next_line(fileno(stdin));
			next_input = ft_strtrim(line, "\n");
			free(line);
		}
		if (!next_input)
		{
			free(input);
			//printf("Exit \n");
			return ;
		}
		if (add_prompt(sh, next_input))
		{
			error_sys("add_prompt failed\n");
			free(input);
			free(next_input);
			return ;
		}
		split_input = split_and_parse(next_input, sh);
		free(next_input);
		if (!split_input)
		{
			error_sys("split_and_parse failed\n");
			free(input);
			sh->exit_status = 1;
			return ;
		}
		next_input = trim_spaces(split_input);
		if (check_syntax(next_input))
		{
			free(input);
			free(next_input);
			sh->exit_status = 2;
			return ;
		}
		split_input = ft_add_spaces(next_input);
		free(next_input);
		if (!split_input)
		{
			error_sys("ft_add_spaces failed\n");
			free(input);
			sh->exit_status = 1;
			return ;
		}
		next_input = split_input;
		if (is_heredoc(next_input))
		{	
			if (handle_here_doc(sh, next_input))
			{
				error_sys("handle_here_doc failed\n");
				free(input);
				free(next_input);
				return ;
			}
		}
		char *temp = ft_strjoin(input, "\n");
		free(input);
		split_input = ft_strjoin(temp, next_input);
		free(temp);
		free(next_input);
		next_input = NULL;
		if (!split_input)
		{
			error_sys("ft_strjoin failed\n");
			sh->exit_status = 1;
			return ;
		}
		input = split_input;
		len = ft_strlen(input);
	}
	if (sh->promt && sh->promt[0] != '\0')
		add_history(sh->promt);
	if (next_input)
		free(next_input);
	sh->commands = split_args_leave_quotes(input, '|');
	free(input);
	if (sh->commands)
	{
		init_num_cmds(sh);
		if (sh->num_cmds == 0)
		{
			sh->exit_status = 0;
			return ;
		}
		execute_pipes(sh);
	}
	else
	{
		error_sys("ft_split failed\n");
		sh->exit_status = 1;
		return ;
	}
}

static int	userprompt(int status, char **envp)
{
	t_shell	sh;
	char	*input;

	initialize_shell(&sh, envp);
	while (1)
	{
		//Snippet for tester
		if (isatty(fileno(stdin)))
			input = readline("minishell> ");
		else
		{
			char *line;
			line = get_next_line(fileno(stdin));
			input = ft_strtrim(line, "\n");
			free(line);
		}
		//input = readline("minishell> ");
		if (input == NULL)
		{
			//printf("Exit \n");
			break ;
		}
		if (add_prompt(&sh, input))
		{
			error_sys("add_prompt failed\n");
			free(input);
			exit(1);
		}
		process_input(&sh, input);
		free_partial(&sh);
		free(input);
	}
	status = sh.exit_status;
	free_shell(&sh);
	rl_clear_history();
	return (status);
}

int	main(int ac, char **av, char **envp)
{
	int	status;

	status = 0;
	(void)ac;
	(void)av;
	return (userprompt(status, envp));
}