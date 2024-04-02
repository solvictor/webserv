# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: vegret <victor.egret.pro@gmail.com>        +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2022/09/24 04:45:01 by dtelnov           #+#    #+#              #
#    Updated: 2024/03/30 01:14:58 by vegret           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv
PROJECT_NAME = webserv

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98 -I ./includes -g3
RM = rm -f

# Utils
ERASE_L = \033[K
CURS_UP = \033[A
SAVE_CURS_POS = \033[s
LOAD_CURS_SAVE = \033[u
BOLD = \033[1m
BLINK = \033[5m

# Reset
NC = \033[0m

# Colors
YELLOW = \033[0;33m
GREEN = \033[0;32m
BLUE = \033[0;34m
RED = \033[0;31m
PURPLE = \033[0;35m
CYAN = \033[0;36m
BLACK = \033[0;30
WHITE = \033[0;37m

# Colors
BYELLOW = \033[1;33m
BGREEN = \033[1;32m
BBLUE = \033[1;34m
BRED = \033[1;31m
BPURPLE = \033[1;35m
BCYAN = \033[1;36m
BBLACK = \033[1;30m
BWHITE = \033[1;37m

# Bg colors
GREEN_BG = \033[48;5;2m

SRC_DIR = sources/

FILES = main Cgi Location ServerConfig Client Logger ServerManager utils file_utils ConfigParser Request Response StringVector

SRCS = $(addprefix $(SRC_DIR), $(addsuffix .cpp, $(FILES)))

OBJS = $(SRCS:.cpp=.o)

# counting files vars
TOTAL = $(words $(SRCS))
FILE_COUNT = 0

# progress bar vars
BAR_COUNT = 0
BAR_PROGRESS = 0
BAR_SIZE = 64
GRADIENT_G := \033[38;5;160m \
			\033[38;5;196m \
			\033[38;5;202m \
			\033[38;5;208m \
			\033[38;5;214m \
			\033[38;5;220m \
			\033[38;5;226m \
			\033[38;5;190m \
			\033[38;5;154m \
			\033[38;5;118m \
			\033[38;5;82m \
			\033[38;5;46m \

GRAD_G_SIZE := 12
GRAD_G_PROG := 0

GRADIENT_B = \033[38;5;2m \
			 \033[38;5; \
			 \033[38;5; \
			 \033[38;5; \
			 \033[38;5; \
			 \033[38;5; \
			 \033[38;5; \
			 \033[38;5; \
			 \033[38;5; \
			 \033[38;5;

define GET_G_GRADIENT
$(word $(1),$(GRADIENT_G))
endef

all: $(NAME)

$(NAME): $(OBJS)
#	==================== draw progress bar ===================
#	=========== erase prev line + write "compiling" ==========
	@echo "$(ERASE_L)$(BOLD)\tCompiling:$(NC)"
#	=============== draw finished progress bar ===============
	@printf "\t█$(GREEN)"
	@for N in $$(seq 1 $(BAR_PROGRESS)); do \
		echo -n █; \
	done
#	============= save position of cursor (eol) ==============
	@printf "$(SAVE_CURS_POS)"
#	======== go back to the middle of the line with \b =======
	@$(eval BAR_PROGRESS=$(shell echo $$(($(BAR_PROGRESS) / 2))))
	@for N in $$(seq 1 $(BAR_PROGRESS)); do \
		echo -n "\b"; \
	done
#	====================== print "DONE" ======================
	@printf "\b\b$(NC)$(BLINK)$(BOLD)$(GREEN_BG)DONE"
#	= go back to the saved position (eol) and go up one line =
	@printf "$(LOAD_CURS_SAVE)$(NC)█$(CURS_UP)"
#	==== go back several characters and print percentage =====
	@printf "\b\b\b\b$(BOLD)%3d%%$(NC)\r" $(PERCENT)
#	================= write rest of messages =================
	@echo "\n\n\n[🔘] $(BGREEN)$(PROJECT_NAME) compiled !$(NC)\n"
	@$(CC) $(CFLAGS) $(OBJS) -o $@
	@printf "[✨] $(BCYAN)[ %d/%d ]\t$(BWHITE)All files have been compiled ✔️$(NC)\n" $(FILE_COUNT) $(TOTAL)
	@echo "[💠] $(BCYAN)$(PROJECT_NAME)\t$(BWHITE) created ✔️\n$(NC)"

%.o: %.cpp
	@$(CC) $(CFLAGS) -c $< -o $@
	@$(eval FILE_COUNT=$(shell echo $$(($(FILE_COUNT)+1))))
	@$(eval PERCENT:=$(shell echo $$((100*$(FILE_COUNT) /$(TOTAL)))))
#	================= calculate progress bar =================
	@$(eval BAR_PROGRESS=$(shell echo $$(($(BAR_SIZE)*$(FILE_COUNT)/$(TOTAL)))))
#	================== calculate bar color ===================
	@$(eval GRAD_G_PROG=$(shell echo $$(($(GRAD_G_SIZE)*$(FILE_COUNT)/$(TOTAL) + 1))))
#	========== printing compiling file + percentage ==========
	@printf "\t$(BWHITE)Compiling: $@%*s...$(NC)$(ERASE_L)\n" $$(($(FILE_COUNT)/$(TOTAL)*33))
#	=================== draw progress bar ====================
	@printf "\t█$(call GET_G_GRADIENT,$(GRAD_G_PROG))"
	@for N in $$(seq 1 $(BAR_PROGRESS)); do \
		echo -n █; \
	done
	@for N in $$(seq 1 $(shell echo $$(($(BAR_SIZE) - $(BAR_PROGRESS))))); do \
		echo -n ░; \
	done
	@printf "$(NC)█$(CURS_UP)"
	@printf "\b\b\b\b$(BOLD)%3d%%$(NC)\r" $(PERCENT)
#	==========================================================
#	@sleep 1

bonus: all

clean:
	@$(RM) $(OBJS)
	@echo "[🧼] $(BYELLOW)Objects $(YELLOW)files have been cleaned from $(PROJECT_NAME) ✔️$(NC)\n"

fclean: clean
	@$(RM) $(NAME)
	@echo "[🚮] $(BRED)All $(RED)files have been cleaned ✔️$(NC)\n"

re: clean all

.PHONY: bonus all clean fclean re
