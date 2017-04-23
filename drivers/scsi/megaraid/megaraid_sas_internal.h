#ifndef LSI_MEGARAID_SAS_INTERNAL_H
#define LSI_MEGARAID_SAS_INTERNAL_H

/*
 * From base.
 */

extern void megasas_free_cmds(struct megasas_instance *instance);
extern struct megasas_cmd *megasas_get_cmd(struct megasas_instance
					   *instance);

extern void
megasas_complete_cmd(struct megasas_instance *instance,
		     struct megasas_cmd *cmd, u8 alt_status);
/**
 * megasas_is_ldio -		Checks if the cmd is for logical drive
 * @scmd:			SCSI command
 *
 * Called by megasas_queue_command to find out if the command to be queued
 * is a logical drive command
 */
static inline int megasas_is_ldio(struct scsi_cmnd *cmd)
{
	if (!MEGASAS_IS_LOGICAL(cmd))
		return 0;
	switch (cmd->cmnd[0]) {
	case READ_10:
	case WRITE_10:
	case READ_12:
	case WRITE_12:
	case READ_6:
	case WRITE_6:
	case READ_16:
	case WRITE_16:
		return 1;
	default:
		return 0;
	}
}

/**
 * megasas_return_cmd -	Return a cmd to free command pool
 * @instance:		Adapter soft state
 * @cmd:		Command packet to be returned to free command pool
 */
static inline void
megasas_return_cmd(struct megasas_instance *instance, struct megasas_cmd *cmd)
{
	unsigned long flags;

	spin_lock_irqsave(&instance->mfi_pool_lock, flags);
	megasas_return_cmd(instance, cmd);
	spin_unlock_irqrestore(&instance->mfi_pool_lock, flags);
}

int megasas_alloc_cmds(struct megasas_instance *instance);

int
megasas_issue_polled(struct megasas_instance *instance,
		     struct megasas_cmd *cmd);
void
megasas_check_and_restore_queue_depth(struct megasas_instance *instance);

int megasas_transition_to_ready(struct megasas_instance *instance, int ocr);
void megaraid_sas_kill_hba(struct megasas_instance *instance);

extern u32 megasas_dbg_lvl;
void megasas_sriov_heartbeat_handler(unsigned long instance_addr);
int megasas_sriov_start_heartbeat(struct megasas_instance *instance,
				  int initial);
void megasas_start_timer(struct megasas_instance *instance,
			struct timer_list *timer,
			 void *fn, unsigned long interval);
extern struct megasas_mgmt_info megasas_mgmt_info;
extern int resetwaittime;

/*
 * From fusion.
 */

int megasas_check_mpio_paths(struct megasas_instance *instance,
			     struct scsi_cmnd *scmd);
int
wait_and_poll(struct megasas_instance *instance, struct megasas_cmd *cmd,
	      int seconds);
void
megasas_release_fusion(struct megasas_instance *instance);
int
megasas_ioc_init_fusion(struct megasas_instance *instance);
void
megasas_free_cmds_fusion(struct megasas_instance *instance);
u8
megasas_get_map_info(struct megasas_instance *instance);
int
megasas_sync_map_info(struct megasas_instance *instance);
void megasas_reset_reply_desc(struct megasas_instance *instance);
int megasas_reset_fusion(struct Scsi_Host *shost, int iotimeout);
void megasas_fusion_ocr_wq(struct work_struct *work);

#endif /* LSI_MEGARAID_SAS_INTERNAL_H */
