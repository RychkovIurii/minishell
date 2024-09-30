/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_pipes.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: irychkov <irychkov@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/09/04 14:25:13 by irychkov          #+#    #+#             */
/*   Updated: 2024/09/15 14:48:32 by irychkov         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

static void	child_io(t_cmd *cmd, int **fd, int i, int num_cmds)
{
	if (cmd->infile)
	{
		dup2(cmd->fd_in, STDIN_FILENO);
		close(cmd->fd_in);
	}
	else if (i > 0)
	{
		close(fd[i - 1][1]);
		dup2(fd[i - 1][0], STDIN_FILENO);
		close(fd[i - 1][0]);
	}
	if (cmd->outfile)
	{
		dup2(cmd->fd_out, STDOUT_FILENO);
		close(cmd->fd_out);
	}
	else if (i < num_cmds - 1)
	{
		close(fd[i][0]);
		dup2(fd[i][1], STDOUT_FILENO);
		close(fd[i][1]);
	}
}

static int	pipe_and_fork(t_shell *sh, t_pipes *pipes, int i, t_cmd *cmd)
{
	if (i < sh->num_cmds - 1)
	{
		if (pipe(pipes->fd[i]) == -1)
		{
			error_sys("pipe failed\n");
			return (1);
		}
	}
	if (sh->num_cmds == 1 && is_builtin(cmd))
	{
		parse_redirections(cmd, cmd->args);
		if (cmd->infile)
		{
			dup2(cmd->fd_in, STDIN_FILENO);
			close(cmd->fd_in);
		}
		if (cmd->outfile)
		{
			dup2(cmd->fd_out, STDOUT_FILENO);
			close(cmd->fd_out);
		}
		if (execute_builtin(sh, cmd))
			return (1);
		return (0);
	}
	pipes->pid[i] = fork();
	if (pipes->pid[i] == -1)
	{
		error_sys("fork failed\n");
		return (1);
	}
	if (pipes->pid[i] == 0)
	{
		parse_redirections(cmd, cmd->args);
		child_io(cmd, pipes->fd, i, sh->num_cmds);
		if (is_builtin(cmd))
		{
			if (execute_builtin(sh, cmd))
				exit(1);
			else
				exit(0);
		}
		execute_command(cmd, sh->envp);
		exit(1);
	}
	if (i > 0)
	{
		close(pipes->fd[i - 1][0]);
		close(pipes->fd[i - 1][1]);
	}
	return (0);
}

/* static void	close_pipes_in_parent(t_pipes *pipes, int num_cmds)
{
	int	i;

	i = 0;
	while (i < num_cmds - 1)
	{
		if (pipes->fd[i][0] != -1)
			close(pipes->fd[i][0]);
		if (pipes->fd[i][1] != -1)
			close(pipes->fd[i][1]);
		i++;
	}
} */

static void	wait_for_children(t_pipes *pipes, t_shell *sh)
{
	int	i;
	int	status;

	i = 0;
	while (i < sh->num_cmds)
	{
		waitpid(pipes->pid[i], &status, 0);
		if (WIFEXITED(status))
			sh->exit_status = WEXITSTATUS(status);
		i++;
	}
}

int	execute_pipes(t_shell *sh)
{
	int		i;
	t_cmd	*cmd;
	t_pipes	pipes;

	i = 0;
	cmd = NULL;
	if (init_pipes(&pipes, sh->num_cmds) == 1)
	{
		error_sys("malloc failed\n");
		return (1);
	}
	while (sh->commands[i] != NULL)
	{
		if (init_cmd(&cmd, sh->commands[i], sh) == 1)
			return (1);
		if (pipe_and_fork(sh, &pipes, i, cmd) != 0)
		{
			free_cmd(cmd);
			return (1);
		}
		free_cmd(cmd);
		i++;
	}
	/* close_pipes_in_parent(&pipes, sh->num_cmds); */
	wait_for_children(&pipes, sh);
	free_pipes(&pipes, sh->num_cmds);
	return (0);
}
