#ifndef  __DIR_LIST_H_
#define  __DIR_LIST_H_

//����dirĿ¼�б����
int dir_list_form_load(char *dir, char *title);

//����/��Ϸ��app������, ���·���dir�б����
void dir_list_form_reload(void);

//����/ͼƬ��app������, ����mode�Զ�����list item
int dir_list_item_auto_run(int mode, char **fname_out);

//��ȡ��ǰѡ�е�item�ļ���
int dir_list_get_cur_item(char **fname_out);


#endif


