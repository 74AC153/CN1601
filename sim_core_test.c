#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>

#include "sim_core.h"

int load_file(char *path, char **data, size_t *datalen)
{
	struct stat sb;
	int status;
	size_t filesize;
	size_t bytesread;
	char *buf = NULL, *cursor;
	int fd = -1;

	fd = open(path, O_RDONLY);
	if((fd = open(path, O_RDONLY)) < 0) {
		fprintf(stderr, "error opening %s: %s (errno=%d)\n",
		        path, strerror(errno), errno);
		goto error;
	}

	if(fstat(fd, &sb)) {
		fprintf(stderr, "error in stat(\"%s\"): %s (errno=%d)\n",
		        path, strerror(errno), errno);
		goto error;
	}
	filesize = sb.st_size;

	if(!(buf = malloc(filesize))) {
		fprintf(stderr, "out of memory\n");
		goto error;
	}

	cursor = buf;
	bytesread = 0;
	while(filesize) {
		status = read(fd, cursor, filesize);
		if(status < 0){
			if(errno != EINTR) {
				fprintf(stderr, "read error: %s (errno=%d)\n",
				        strerror(errno), errno);
				goto error;
			}
		}
		if(status == 0) {
			fprintf(stderr, "Warning: unexpected end of file at byte %lu\n",
			        bytesread);
			break;
		}
		filesize -= status;
		cursor += status;
		bytesread += status;
	}

	*data = buf;
	*datalen = bytesread;
	close(fd);
	return 0;

error:
	free(buf);
	if(fd >= 0) {
		close(fd);
	}
	return -1;
}


typedef struct {
	char *init_state;
	char *exec_input;
	char *postexec_state;
	char *exec_output;
	char *update_input;
	char *postupdate_state;
	char *update_output;
} cli_opts_t;


void usage()
{
	printf(
"usage:\n"
"[-x <init state>]        default: all bits reset\n"
" -a <exec input>\n"
"[-y <post exec state>]   default: not compared\n"
"[-o <exec output>]       default: not compared\n"
" -b <update input>\n"
" -z <post update state>\n"
" -p <update output>\n"
"\n"
"operations performed:\n"
"core-state <- init state\n"
"core-input <- exec input\n"
"\n"
"sim_core_exec()\n"
"\n"
"core-state =? post exec state\n"
"core-output =? exec output \n"
"\n"
"core-input <- update input \n"
"\n"
"sim_core_update()\n"
"\n"
"core-state =? post update state \n"
"core-output =? update output\n");
}


int parse_cli(int argc, char *argv[], cli_opts_t *opts)
{
	int ch;
	memset(opts, 0, sizeof(*opts));
	char *buf;

	while((ch = getopt(argc, argv, "x:a:y:o:b:z:p:h")) != -1) {
		switch(ch) {
		case 'x': opts->init_state = optarg; break;
		case 'a': opts->exec_input = optarg; break;
		case 'y': opts->postexec_state = optarg; break;
		case 'o': opts->exec_output = optarg; break;
		case 'b': opts->update_input = optarg; break;
		case 'z': opts->postupdate_state = optarg; break;
		case 'p': opts->update_output = optarg; break;
		default:  fprintf(stderr, "unknown switch: %c  run with -h\n", ch);
		case 'h': return -1;
		}
	}

	if(opts->exec_input == NULL) {
		fprintf(stderr, "-a <exec input> required\n");
		return -1;
	}
	if(opts->update_input == NULL) {
		fprintf(stderr, "-b <update input> required\n");
		return -1;
	}
	if(opts->postupdate_state == NULL) {
		fprintf(stderr, "-z <post update state> required\n");
		return -1;
	}
	if(opts->update_output == NULL) {
		fprintf(stderr, "-p <update output> required\n");
		return -1;
	}

	return 0;
}

int read_eq_semicolon_pair(
	char *desc, size_t desclen,
	char *namebuf, size_t namebuflen,
	char *valbuf, size_t valbuflen)
{
	/*
	desc = "  alnums_name   =   alnums_value   \nanything.."
	namebuf <- "alnums_name"
	valbuf <- "alnums_value"
	return: strstr(desc, "anything") - desc
	*/
	char *eq, *term, *start, *end;
	if((eq = strchr(desc, '=')) == NULL) {
		return 0;
	}
	if((term = strchr(desc, ';')) == NULL) {
		return 0;
	}

	for(start = desc; !isalnum(*start); start++);
	for(end = eq; !isalnum(*end); end--);
	end++;
	strncpy(namebuf, start, end-start);
	namebuf[end-start] = 0;

	for(start = eq; !isalnum(*start); start++);
	for(end = term; !isalnum(*end); end--);
	end++;
	strncpy(valbuf, start, end - start);
	valbuf[end-start] = 0;

	term++;
	return term - desc;
}

int parse_statedesc(char *desc, size_t desclen, sim_core_state_t *state)
{
	char namebuf[32], valbuf[32];
	int nread;

	while(nread = read_eq_semicolon_pair(desc, desclen,
	                                     namebuf, sizeof(namebuf),
	                                     valbuf, sizeof(valbuf))) {
		if(strcmp(namebuf, "pc") == 0) {
			state->pc = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_super0") == 0) {
			state->gpr_super[0] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_super1") == 0) {
			state->gpr_super[1] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_super2") == 0) {
			state->gpr_super[2] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_super3") == 0) {
			state->gpr_super[3] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_super4") == 0) {
			state->gpr_super[4] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_super5") == 0) {
			state->gpr_super[5] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_super6") == 0) {
			state->gpr_super[6] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_super7") == 0) {
			state->gpr_super[7] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_user0") == 0) {
			state->ctl.named.gpr_user[0] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_user1") == 0) {
			state->ctl.named.gpr_user[1] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_user2") == 0) {
			state->ctl.named.gpr_user[2] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_user3") == 0) {
			state->ctl.named.gpr_user[3] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_user4") == 0) {
			state->ctl.named.gpr_user[4] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_user5") == 0) {
			state->ctl.named.gpr_user[5] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_user6") == 0) {
			state->ctl.named.gpr_user[6] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "gpr_user7") == 0) {
			state->ctl.named.gpr_user[7] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "epc") == 0) {
			state->ctl.named.epc = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "status_swiarg") == 0) {
			state->ctl.named.status.swiarg = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "status_ret_umode") == 0) {
			state->ctl.named.status.ret_umode = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "status_glob_int_en") == 0) {
			state->ctl.named.status.glob_int_en = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "status_in_um") == 0) {
			state->ctl.named.status.in_um = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "ifaulthdl") == 0) {
			state->ctl.named.ifaulthdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "dfaulthdl") == 0) {
			state->ctl.named.dfaulthdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "swihdl") == 0) {
			state->ctl.named.swihdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "illophdl") == 0) {
			state->ctl.named.illophdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "ealuhdl") == 0) {
			state->ctl.named.ealuhdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exi0hdl") == 0) {
			state->ctl.named.exi0hdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exi1hdl") == 0) {
			state->ctl.named.exi1hdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exi2hdl") == 0) {
			state->ctl.named.exi2hdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exi3hdl") == 0) {
			state->ctl.named.exi3hdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exi4hdl") == 0) {
			state->ctl.named.exi4hdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exi5hdl") == 0) {
			state->ctl.named.exi5hdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exi6hdl") == 0) {
			state->ctl.named.exi6hdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exi0hdl") == 0) {
			state->ctl.named.exi7hdl = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "ienable") == 0) {
			state->ctl.named.ienable= strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "ipend") == 0) {
			state->ctl.named.ipend= strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "iack") == 0) {
			state->ctl.named.iack= strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "umcpen") == 0) {
			state->ctl.named.umcpen= strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "counter") == 0) {
			state->ctl.named.counter = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "ctlunused0") == 0) {
			state->ctl.named.unused[0] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "ctlunused1") == 0) {
			state->ctl.named.unused[1] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "ctlunused2") == 0) {
			state->ctl.named.unused[2] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "ctlunused3") == 0) {
			state->ctl.named.unused[3] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "aluresult") == 0) {
			state->aluresult = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "destgpreg") == 0) {
			state->destgpreg = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "nextpc") == 0) {
			state->nextpc = strtol(valbuf, NULL, 0);
		} else {
			fprintf(stderr, "unknown state key: %s\n", namebuf);
			return -1;
		}
		desc += nread;
		desclen -= nread;
	}
	return 0;
}

int parse_inputdesc(char *desc, size_t desclen, sim_core_input_t *input)
{
	char namebuf[32], valbuf[32];
	int nread;

	while(nread = read_eq_semicolon_pair(desc, desclen,
	                                     namebuf, sizeof(namebuf),
	                                     valbuf, sizeof(valbuf))) {
		if(strcmp(namebuf, "data_value") == 0) {
			input->data_value = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "data_valid") == 0) {
			input->data_valid = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "data_fault") == 0) {
			input->data_fault = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "instr_value") == 0) {
			input->instr_value = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "instr_valid") == 0) {
			input->instr_valid = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "instr_fault") == 0) {
			input->instr_fault = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "coproc_value") == 0) {
			input->coproc_value = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_sig0") == 0) {
			input->exint_sig[0] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_sig1") == 0) {
			input->exint_sig[1] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_sig2") == 0) {
			input->exint_sig[2] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_sig3") == 0) {
			input->exint_sig[3] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_sig4") == 0) {
			input->exint_sig[4] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_sig5") == 0) {
			input->exint_sig[5] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_sig6") == 0) {
			input->exint_sig[6] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_sig7") == 0) {
			input->exint_sig[7] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "ll_inval") == 0) {
			input->ll_inval = strtol(valbuf, NULL, 0);
		} else {
			fprintf(stderr, "unknown input key: %s\n", namebuf);
			return -1;
		}
		desc += nread;
		desclen -= nread;
	}
	return 0;
}

int parse_outputdesc(char *desc, size_t desclen, sim_core_output_t *output)
{
	char namebuf[32], valbuf[32];
	int nread;

	while(nread = read_eq_semicolon_pair(desc, desclen,
	                                     namebuf, sizeof(namebuf),
	                                     valbuf, sizeof(valbuf))) {
		if(strcmp(namebuf, "data_value") == 0) {
			output->data_value = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "data_addr") == 0) {
			output->data_addr = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "data_read") == 0) {
			output->data_read = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "data_request") == 0) {
			output->data_request = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "data_fault_ack") == 0) {
			output->data_fault_ack = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "instr_addr") == 0) {
			output->instr_addr = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "instr_request") == 0) {
			output->instr_request = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "instr_fault_ack") == 0) {
			output->instr_fault_ack = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "coproc_select") == 0) {
			output->coproc_select = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "coproc_op") == 0) {
			output->coproc_op = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "coproc_addr") == 0) {
			output->coproc_addr = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "coproc_value") == 0) {
			output->coproc_value = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_ack0") == 0) {
			output->exint_ack[0] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_ack1") == 0) {
			output->exint_ack[1] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_ack2") == 0) {
			output->exint_ack[2] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_ack3") == 0) {
			output->exint_ack[3] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_ack4") == 0) {
			output->exint_ack[4] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_ack5") == 0) {
			output->exint_ack[5] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_ack6") == 0) {
			output->exint_ack[6] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "exint_ack7") == 0) {
			output->exint_ack[7] = strtol(valbuf, NULL, 0);
		} else if(strcmp(namebuf, "user_mode") == 0) {
			output->exint_ack[7] = strtol(valbuf, NULL, 0);
		} else {
			fprintf(stderr, "unknown output key: %s\n", namebuf);
			return -1;
		}
		desc += nread;
		desclen -= nread;
	}
	return 0;
}

int read_input_desc(char *path, sim_core_input_t *input)
{
	char *buf;
	size_t buflen;
	int status;

	if(load_file(path, &buf, &buflen)) {
		return -1;
	}

	memset(input, 0, sizeof(*input));

	if(parse_inputdesc(buf, buflen, input)) {
		return -1;
	}

	return 0;
}

int read_output_desc(char *path, sim_core_output_t *output)
{
	char *buf;
	size_t buflen;
	int status;

	if(load_file(path, &buf, &buflen)) {
		return -1;
	}

	memset(output, 0, sizeof(*output));

	if(parse_outputdesc(buf, buflen, output)) {
		return -1;
	}

	return 0;
}

int read_state_desc(char *path, sim_core_state_t *state)
{
	char *buf;
	size_t buflen;
	int status;

	if(load_file(path, &buf, &buflen)) {
		return -1;
	}

	memset(state, 0, sizeof(*state));

	if(parse_statedesc(buf, buflen, state)) {
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	sim_core_state_t state, inter_state, final_state;
	sim_core_input_t input;
	sim_core_output_t output;
	uint16_t *prog;
	size_t proglen;
	cli_opts_t opts;
	int status;
	
	char *buf;
	size_t buflen;
	sim_core_state_t init_state;
	sim_core_input_t exec_input;
	sim_core_state_t postexec_state;
	sim_core_output_t exec_output;
	sim_core_input_t update_input;
	sim_core_state_t postupdate_state;
	sim_core_output_t update_output;

	sim_core_state_t core_state;
	sim_core_input_t core_input;
	sim_core_output_t core_output;

	/* read cli args */
	if(parse_cli(argc, argv, &opts)) {
		usage();
		return -1;
	}

	/* load files */
	if(read_state_desc(opts.init_state, &init_state)) {
		return -1;
	}
	if(read_input_desc(opts.exec_input, &exec_input)) {
		return -1;
	}
	if(read_state_desc(opts.postexec_state, &postexec_state)) {
		return -1;
	}
	if(read_output_desc(opts.exec_output, &exec_output)) {
		return -1;
	}
	if(read_input_desc(opts.update_input, &update_input)) {
		return -1;
	}
	if(read_state_desc(opts.postupdate_state, &postupdate_state)) {
		return -1;
	}
	if(read_output_desc(opts.update_output, &update_output)) {
		return -1;
	}
	
	/* initialize the core */
	memcpy(&core_state, &init_state, sizeof(core_state));
	memcpy(&core_input, &exec_input, sizeof(core_input));
	memset(&core_output, 0, sizeof(core_output));

	/* run exec */
	status = sim_core_exec(&core_state, &core_output, &core_input);
	if(status) {
		fprintf(stderr, "error: sim_core_exec()\n");
		return status;
	}

	/* validate state and output */
	status = memcmp(&postexec_state, &core_state, sizeof(postexec_state));
	if(status) {
		fprintf(stderr, "error: core_state does not match postexec_state\n");
		return status;
	}
	status = memcmp(&exec_output, &core_output, sizeof(exec_output));
	if(status) {
		fprintf(stderr, "error: core_output does not match exec_output\n");
		return status; 
	}

	/* update input */
	memcpy(&core_input, &update_input, sizeof(update_input));

	/* run update */
	status = sim_core_update(&core_state, &core_output, &core_input);

	/* validate state and output */
	status = memcmp(&postupdate_state, &core_state, sizeof(postupdate_state));
	if(status) {
		fprintf(stderr, "error: core_state does not match postupdate_state\n");
		return status;
	}
	status = memcmp(&update_output, &core_output, sizeof(update_output));
	if(status) {
		fprintf(stderr, "error: core_output does not match update_output\n");
		return status;
	}

	return 0;
}
