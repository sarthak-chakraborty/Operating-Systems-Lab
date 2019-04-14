#include "alt2.h"

#define SAME_DIR -5
#define REGULAR 1
#define DIRECTORY 2

using namespace std;


// Declerations
long long int file_size, block_size, n_blocks;
file_sys file;
vector<FD_t> FD;

int cur_dir;
int prev_dir;


/////////////////////////////////////////////////////////////////////////////////////////
void init(long long int a, long long int b, long long int c)
{
	::file_size = a;
	::block_size = b;
	::n_blocks = c;

	int i;
	//cout<<"Here......"<<endl;
	file.sp.file_sys_size = a;
	file.sp.block_size = b;
	file.sp.n_blocks = c;
	file.sp.n_inodes = 2 * (b/sizeof(inode));
	strcpy(file.sp.name, "INODE");
	file.sp.first_free_block = 3;
	
	file.b_inode.iNode = (inode *)malloc(file.sp.n_inodes * sizeof(inode));
	file.sp.free_inode=(bool*)malloc(file.sp.n_inodes * sizeof(bool));
	// for(i=0; i<file.sp.n_inodes; i++)
	// 	file.b_inode.iNode[i].free = true;
	// Populate free_inode list
	bool topush=true;
	
	for(i=0; i<file.sp.n_inodes; i++)
		file.sp.free_inode[i]=true;
	
	file.b = (data_block *)malloc((c-3)*sizeof(data_block));
	
	for(i=0; i<(c-3); i++)
	{
		file.b[i].next = i+4;
		file.b[i].data = (char *)malloc((b*1024)*sizeof(char));
		for(int j = 0; j<1024; j++)
			file.b[i].data[j] = '\0';
	}
	
	// file.b_inode.iNode[0].free = false;
	// file.b_inode.iNode[0].type = false;
	
	file.sp.free_inode[0] = false;
	//cout<<"\nHere..."<<endl;
	int tp=-1;
	//for(int i=0; i<5; i++)
	//	file.b_inode.iNode[0].dp.push_back(tp);

	// LAST DATA BLOCK NOT INITIALIkeZED
	file.b_inode.iNode[0].dp.resize(5,tp);
	file.b_inode.iNode[0].dp[0] = SAME_DIR;
	file.b_inode.iNode[0].dp[1] = file.sp.first_free_block;
	file.sp.first_free_block = file.b[file.sp.first_free_block-3].next;
	file.b_inode.iNode[0].sip=file.b_inode.iNode[0].dip=-1;
	cur_dir = 0;
	prev_dir=-1;
}

/////////////////////////////////////////////////////////////////////////////////////////
int getpointertonextentry(int inodeno, int moveby, int* currentdatablockno, int* currentdatablockoffset, int* indirectiontype, int* directoffset, int* sipblockoffset, int* dipblockoffset1, int* dipblockoffset2 )
{

	if(*currentdatablockoffset+moveby<block_size){
		*currentdatablockoffset=*currentdatablockoffset+moveby;
		if(file.b[*(currentdatablockno)-3].data[*currentdatablockoffset]==NULL) return (-3);
		return (*indirectiontype);
	}
	else if(*currentdatablockno==file.b_inode.iNode[inodeno].last_data_block) // YE TO HOGA HI
	{
		return -1;
	}
	else if (*indirectiontype==0 && *directoffset<4)
	{
		*directoffset++;
		*currentdatablockno=file.b_inode.iNode[inodeno].dp[*directoffset];
		*currentdatablockoffset=0;
		return 0;
	}
	else if (*indirectiontype==0 && *directoffset==4)
	{
		*indirectiontype=1;
		*sipblockoffset=0;
		int* ptr=(int*)file.b[file.b_inode.iNode[inodeno].sip-3].data;
		*currentdatablockno=*ptr;
		*currentdatablockoffset=0;
		return (*indirectiontype);
	}
	else if(*indirectiontype==1 && (*sipblockoffset+sizeof(int))<block_size)
	{
		*sipblockoffset+=sizeof(int);
		int* ptr=(int*)((char*)file.b[file.b_inode.iNode[inodeno].sip-3].data+sizeof(int));
		*currentdatablockno=*ptr;
		*currentdatablockoffset=0;
		return (*indirectiontype);
	}
	else if(*indirectiontype==1 && (*sipblockoffset+sizeof(int))>=block_size)
	{
		*indirectiontype=2;
		*dipblockoffset1=0;
		*dipblockoffset2=0;
		int dipblockno=file.b_inode.iNode[inodeno].dip;
		int* tempptr=(int*)((char*)file.b[dipblockno-3].data+*dipblockoffset1);
		int actualsearchinblock=*tempptr;
		int* tempptr2=(int*)((char*)file.b[actualsearchinblock-3].data+*dipblockoffset2);
		*currentdatablockno=*tempptr2;
		*currentdatablockoffset=0;
		return (*indirectiontype);
	}
	else if(*indirectiontype==2 && (*dipblockoffset2+sizeof(int))<block_size)
	{
		*dipblockoffset2+=sizeof(int);
		int dipblockno=file.b_inode.iNode[inodeno].dip;
		int* tempptr=(int*)((char*)file.b[dipblockno-3].data+*dipblockoffset1);
		int actualsearchinblock=*tempptr;
		int* tempptr2=(int*)((char*)file.b[actualsearchinblock-3].data+*dipblockoffset2);
		*currentdatablockno=*tempptr2;
		*currentdatablockoffset=0;
		return (*indirectiontype);
	}
	else if(*indirectiontype==2 && (*dipblockoffset2+sizeof(int))>=block_size && (*dipblockoffset1+sizeof(int))<block_size)
	{
		*dipblockoffset1+=sizeof(int);
		*dipblockoffset2=0;
		int dipblockno=file.b_inode.iNode[inodeno].dip;
		int* tempptr=(int*)((char*)file.b[dipblockno-3].data+*dipblockoffset1);
		int actualsearchinblock=*tempptr;
		int* tempptr2=(int*)((char*)file.b[actualsearchinblock-3].data+*dipblockoffset2);
		*currentdatablockno=*tempptr2;
		*currentdatablockoffset=0;
		return (*indirectiontype);
	}
	else
	{
		// cout<<"The file system is full"<<endl;
		return -2;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
int my_mkdir(char *str)
{
	// cout<<"Here......"<<endl;
	int inodeno = cur_dir; //jobhi inode we are working on
	int currentdatablockno;
	if(inodeno==0)
		currentdatablockno = file.b_inode.iNode[cur_dir].dp[1];  //initialize to dp[0] wala block no
	else
		currentdatablockno = file.b_inode.iNode[cur_dir].dp[2];
	// cout<<currentdatablockno<<endl;
	int currentdatablockoffset = 0; //initially 0
	int indirectiontype = 0; //initially 0
	int directoffset = 0; //initially 0
	int sipblockoffset = 0; //initially 0
	int dipblockoffset1 = 0; //initially 0
	int dipblockoffset2 = 0; //initially 0
	// cout<<"Here......"<<endl;

	//search if same named directory/file exists, if no return -1
	int found=0;
	int rv;
	int foundinodeno;
	// cout<<"Trying to search if same named files exist"<<endl;
	do
	{
		char filename[15];
		for(int i=0;i<14;i++){
			filename[i]=file.b[currentdatablockno-3].data[currentdatablockoffset+i];
		}
		filename[14]='\0';
		if(!strcmp(filename,str))
		{
			found=1;
			int foundinodeno=*(short int*)(file.b[currentdatablockno-3].data+(int)(currentdatablockoffset)+14);
			break;
		}
		rv=getpointertonextentry(cur_dir, 16, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);
	}while(rv!=-1 && rv!=-3);
	//if that file is a dir, return -2
	// cout<<"found : "<<found<<endl;
	// if(file.b_inode.iNode[foundinodeno].type==DIRECTORY)
		// return -2;

	//else add a new entry in this directory
	if (found==0)
	{
		if(currentdatablockoffset+16<block_size)
		{
			for(int i=0;i<14;i++)
			{
				file.b[currentdatablockno-3].data[currentdatablockoffset+i]=str[i];
			}

		}
		short int freeinode = -1;
		for(int i=0; i<file.sp.n_inodes; i++)
		{
			if(file.sp.free_inode[i] == true)
			{
				freeinode = (short int)i;
				break;
			}
		}
		// cout<<"free inode found : "<<freeinode<<endl;
		if(freeinode == -1)
			return -1;
		file.b_inode.iNode[freeinode].dp.resize(5,-1);
		//FD[i].current_block=file.b_inode.iNode[freeinode].dp[0];
		// cout<<"1.Hopefully we are reaching here"<<endl;
		file.b_inode.iNode[freeinode].dp[0]=SAME_DIR;
		// cout<<"2.Hopefully we are reaching here"<<endl;
		file.b_inode.iNode[freeinode].dp[1]=cur_dir;
		// cout<<"3.Hopefully we are reaching here"<<endl;
		file.b_inode.iNode[freeinode].dp[2]=file.sp.first_free_block;
		// cout<<"4.Hopefully we are reaching here"<<endl;
		file.b_inode.iNode[freeinode].last_data_block=file.sp.first_free_block;
		// cout<<"5.Hopefully we are reaching here"<<endl;
		file.sp.first_free_block=file.b[file.sp.first_free_block-3].next;
		file.sp.free_inode[freeinode]=false;
		char* ptr=file.b[currentdatablockno-3].data+currentdatablockoffset+14;
		// cout<<"trying to write at currentblock : "<<currentdatablockno<<" and offset : "<<currentdatablockoffset<<endl;
		// cout<<"Writing inode no : "<<freeinode<<endl;
		sprintf(ptr,"%02hd",(short int)freeinode);
		for(int i=0; i<2; i++)
		{
			// ptr[i] = file.b[currentdatablockno].data[currentdatablockoffset+14+i];
			file.b[currentdatablockno].data[currentdatablockoffset+14+i]=ptr[i];
			// cout<<file.b[currentdatablockno].data[currentdatablockoffset+14+i]<<endl;
		}
		// cout<<"1."<<file.b[currentdatablockno-3].data+currentdatablockoffset+14<<endl;
		// cout<<"2."<<file.b[currentdatablockno-3].data+currentdatablockoffset+15;
		// cout<<"3."<<file.b[currentdatablockno-3].data+currentdatablockoffset+16<<endl;



	}
	else
	{
		cout<<"Same name file/directory already exists in the current directory"<<endl;
		return -1;
	}

}

/////////////////////////////////////////////////////////////////////////////////////////
int my_rmdir(char *str)
{
	// cout<<"Here......"<<endl;
	int inodeno = cur_dir; //jobhi inode we are working on
	int currentdatablockno;
	if(inodeno==0)
		currentdatablockno = file.b_inode.iNode[cur_dir].dp[1];  //initialize to dp[0] wala block no
	else
		currentdatablockno = file.b_inode.iNode[cur_dir].dp[2];
	// cout<<currentdatablockno<<endl;
	int currentdatablockoffset = 0; //initially 0
	int indirectiontype = 0; //initially 0
	int directoffset = 0; //initially 0
	int sipblockoffset = 0; //initially 0
	int dipblockoffset1 = 0; //initially 0
	int dipblockoffset2 = 0; //initially 0
	// cout<<"Here......"<<endl;

	//search if same named directory/file exists, if no return -1
	int found=0;
	int rv;
	int foundinodeno;
	// cout<<"Trying to search if same named files exist"<<endl;
	do
	{
		char filename[15];
		for(int i=0;i<14;i++){
			filename[i]=file.b[currentdatablockno-3].data[currentdatablockoffset+i];
		}
		filename[14]='\0';
		if(!strcmp(filename,str))
		{
			found=1;
			int foundinodeno=*(short int*)(file.b[currentdatablockno-3].data+(int)(currentdatablockoffset)+14);
			break;
		}
		rv=getpointertonextentry(cur_dir, 16, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);
	}while(rv!=-1 && rv!=-3);
	//if that file is a dir, return -2
	// cout<<"found : "<<found<<endl;
	// if(file.b_inode.iNode[foundinodeno].type==DIRECTORY)
		// return -2;
	int dirtodelete;
	//else add a new entry in this directory
	if (found==1)
	{
		// prev_dir=cur_dir;
		// cout<<"found at current data block : "<<currentdatablockno<<" and offset : "<<currentdatablockoffset<<endl;
		char ptr[3];
		for(int i=0; i<2; i++)
		{
			ptr[i] = file.b[currentdatablockno].data[currentdatablockoffset+14+i];
			// cout<<ptr[i]<<endl;
		}
		// memcpy(ptr, file.b[currentdatablockno].data+currentdatablockoffset+14, 2);
		ptr[2] = '\0';
		// cout<<ptr<<endl;
		dirtodelete = atoi(ptr);
		for(int i=0;i<16;i++)
			file.b[currentdatablockno-3].data[currentdatablockoffset+i]=-2;
		int inodeno1 = dirtodelete; //jobhi inode we are working on
		int _currentdatablockno;
		if (inodeno1==0)
			_currentdatablockno = file.b_inode.iNode[inodeno1].dp[1];  //initialize to dp[0] wala block no
		else
			_currentdatablockno = file.b_inode.iNode[inodeno1].dp[2];
		// cout<<_currentdatablockno<<endl;
		int _currentdatablockoffset = 0; //initially 0
		int _indirectiontype = 0; //initially 0
		int _directoffset = 0; //initially 0
		int _sipblockoffset = 0; //initially 0
		int _dipblockoffset1 = 0; //initially 0
		int _dipblockoffset2 = 0; //initially 0
		// cout<<"Here......"<<endl;

		//search if same named file exists, if no return -1
		int rv;
		int foundinodeno;
		do
		{
			// char filename[15];
			// for(int i=0;i<14;i++)
			// {
			// 	filename[i]=file.b[currentdatablockno-3].data[currentdatablockoffset+i];
			// }
			// filename[14]='\0';
			// if(!strcmp(filename,str))
			// {
			// 	found=1;
			// 	int foundinodeno=*(short int*)(file.b[currentdatablockno-3].data+(int)(currentdatablockoffset)+14);
			// 	break;
			// }
			// char ptr[3];
			// for(int i=0; i<2; i++)
			// {
			// 	ptr[i] = file.b[_currentdatablockno].data[_currentdatablockoffset+14+i];
			// 	cout<<ptr[i]<<endl;
			// }
			// // memcpy(ptr, file.b[currentdatablockno].data+currentdatablockoffset+14, 2);
			// ptr[2] = '\0';
			// cout<<ptr<<endl;
			// dirtodelete = atoi(ptr);
			for(int i=0;i<16;i++)
				file.b[_currentdatablockno-3].data[_currentdatablockoffset+i]=-2;
			rv=getpointertonextentry(inodeno1, 16, &_currentdatablockno, &_currentdatablockoffset, &_indirectiontype, &_directoffset, &_sipblockoffset, &_dipblockoffset1, &_dipblockoffset2);
		}while(rv!=-1 && rv!=-3);
		// char* ptr=file.b[currentdatablockno].data+currentdatablockoffset+14;
		// sscanf(ptr,"%hd",&cur_dir);
		// cur_dir=*((short int*)(file.b[currentdatablockno].data+currentdatablockoffset+14));
		// cout<<"New current dir should be : "<<cur_dir<<endl;
		return cur_dir;
	}
	else
	{
		cout<<"Same name file/directory does not exist in the current directory"<<endl;
		return -1;
	}

}

////////////////////////////////////////////////////////////////////////////////////////
int my_chdir(char *str)
{
	// cout<<"Here......"<<endl;
	int inodeno = cur_dir; //jobhi inode we are working on
	int currentdatablockno;
	if(inodeno==0)
		currentdatablockno = file.b_inode.iNode[cur_dir].dp[1];  //initialize to dp[0] wala block no
	else
		currentdatablockno = file.b_inode.iNode[cur_dir].dp[2];
	// cout<<currentdatablockno<<endl;
	int currentdatablockoffset = 0; //initially 0
	int indirectiontype = 0; //initially 0
	int directoffset = 0; //initially 0
	int sipblockoffset = 0; //initially 0
	int dipblockoffset1 = 0; //initially 0
	int dipblockoffset2 = 0; //initially 0
	// cout<<"Here......"<<endl;

	//search if same named directory/file exists, if no return -1
	int found=0;
	int rv;
	int foundinodeno;
	// cout<<"Trying to search if same named files exist"<<endl;
	do
	{
		char filename[15];
		for(int i=0;i<14;i++){
			filename[i]=file.b[currentdatablockno-3].data[currentdatablockoffset+i];
		}
		filename[14]='\0';
		if(!strcmp(filename,str))
		{
			found=1;
			int foundinodeno=*(short int*)(file.b[currentdatablockno-3].data+(int)(currentdatablockoffset)+14);
			break;
		}
		rv=getpointertonextentry(cur_dir, 16, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);
	}while(rv!=-1 && rv!=-3);
	//if that file is a dir, return -2
	// cout<<"found : "<<found<<endl;
	// if(file.b_inode.iNode[foundinodeno].type==DIRECTORY)
		// return -2;

	//else add a new entry in this directory
	if (found==1)
	{
		prev_dir=cur_dir;
		// cout<<"found at current data block : "<<currentdatablockno<<" and offset : "<<currentdatablockoffset<<endl;
		char ptr[3];
		for(int i=0; i<2; i++){
			ptr[i] = file.b[currentdatablockno].data[currentdatablockoffset+14+i];
			// cout<<ptr[i]<<endl;
		}
		// memcpy(ptr, file.b[currentdatablockno].data+currentdatablockoffset+14, 2);
		ptr[2] = '\0';
		// cout<<ptr<<endl;
		cur_dir = atoi(ptr);
		// char* ptr=file.b[currentdatablockno].data+currentdatablockoffset+14;
		// sscanf(ptr,"%hd",&cur_dir);
		// cur_dir=*((short int*)(file.b[currentdatablockno].data+currentdatablockoffset+14));
		// cout<<"New current dir should be : "<<cur_dir<<endl;
		return cur_dir;
	}
	else
	{
		cout<<"Same name file/directory does not exist in the current directory"<<endl;
		return -1;
	}

}

/////////////////////////////////////////////////////////////////////////////////////////
int my_open(char *str)
{
	// cout<<"Here......"<<endl;
	int inodeno = cur_dir; //jobhi inode we are working on
	int currentdatablockno ;
	if (cur_dir==0)
		currentdatablockno = file.b_inode.iNode[cur_dir].dp[1];  //initialize to dp[0] wala block no
	else
		currentdatablockno = file.b_inode.iNode[cur_dir].dp[2];
	// cout<<currentdatablockno<<endl;
	int currentdatablockoffset = 0; //initially 0
	int indirectiontype = 0; //initially 0
	int directoffset = 0; //initially 0
	int sipblockoffset = 0; //initially 0
	int dipblockoffset1 = 0; //initially 0
	int dipblockoffset2 = 0; //initially 0
	// cout<<"Here......"<<endl;

	//search if same named file exists, if no return -1
	int found=0;
	int rv;
	int foundinodeno;
	do
	{
		char filename[15];
		for(int i=0;i<14;i++){
			filename[i]=file.b[currentdatablockno-3].data[currentdatablockoffset+i];
		}
		filename[14]='\0';
		if(!strcmp(filename,str))
		{
			found=1;
			int foundinodeno=*(short int*)(file.b[currentdatablockno-3].data+(int)(currentdatablockoffset)+14);
			break;
		}
		rv=getpointertonextentry(cur_dir, 16, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);
	}while(rv!=-1 && rv!=-3);
	//if that file is a dir, return -2
	// cout<<found<<endl;
	// if(file.b_inode.iNode[foundinodeno].type==DIRECTORY)
		// return -2;

	//else 'make' a new FD table entry and return its index
	if (found==0)
	{
		// cout<<"Did not find a matching entry"<<endl;
		for(int j=0;j<FD.size();j++)
		{
			// cout<<"Searching thru present table"<<endl;
			if(FD[j].valid==0)
			{
				// cout<<"Found an empty fd"<<endl;
				FD[j].valid=1;
				//FD[i].dir_no=cur_dir;
				short int freeinode = -1;
				for(int i=0; i<file.sp.n_inodes; i++)
				{
					if(file.sp.free_inode[i] == true)
					{
						freeinode = (short int)i;
						break;
					}
				}
				// cout<<freeinode<<endl;
				if(freeinode == -1)
					return -1;
				FD[j].inode_no = freeinode;
				file.b_inode.iNode[freeinode].dp.resize(5,-1);
				file.b_inode.iNode[freeinode].dp[0]=file.sp.first_free_block;
				file.b_inode.iNode[freeinode].last_data_block=file.sp.first_free_block;
				file.sp.first_free_block=file.b[file.sp.first_free_block-3].next;
				file.sp.free_inode[freeinode]=false;
				FD[j].current_block=file.b_inode.iNode[freeinode].dp[0];
				FD[j].current_offset=0;
				FD[j].current_wblock=file.b_inode.iNode[freeinode].dp[0];
				FD[j].current_woffset=0;
				return j;
			}
		}
		FD_t tempfd;
		tempfd.valid=1;
		//tempfd.dir_no=cur_dir;
		short int freeinode = -1;
		int i;
		for(i=0; i<file.sp.n_inodes; i++)
		{
			if(file.sp.free_inode[i] == true)
			{
				freeinode = (short int)i;
				break;
			}
		}

		if(freeinode == -1)
			return -1;

		tempfd.inode_no = freeinode;
		file.b_inode.iNode[freeinode].dp.resize(5,-1);
		file.b_inode.iNode[freeinode].dp[0]=file.sp.first_free_block;
		file.b_inode.iNode[freeinode].last_data_block = file.sp.first_free_block;

		tempfd.current_block=file.b_inode.iNode[freeinode].dp[0];
		tempfd.current_offset=0;

		file.sp.first_free_block=file.b[file.sp.first_free_block-3].next;
		file.sp.free_inode[freeinode]=false;

		tempfd.current_wblock=file.b_inode.iNode[freeinode].dp[0];
		tempfd.current_woffset=0;

		FD.push_back(tempfd);
		return (FD.size()-1);
	}
	else
	{
		//create free inode
		int i;
		for(i=0;i<FD.size();i++)
		{
			if(FD[i].valid==0)
			{
				FD[i].valid=1;
				//FD[i].dir_no=cur_dir;
				short int freeinode = foundinodeno;
				FD[i].inode_no = freeinode;

				file.b_inode.iNode[freeinode].dp.resize(5,-1);
				file.b_inode.iNode[freeinode].dp[0]=file.sp.first_free_block;

				FD[i].current_block=file.b_inode.iNode[freeinode].dp[0];
				file.b_inode.iNode[freeinode].dp[0]=file.sp.first_free_block;
				file.b_inode.iNode[freeinode].last_data_block=file.sp.first_free_block;
				file.sp.first_free_block=file.b[file.sp.first_free_block-3].next;
				file.sp.free_inode[freeinode]=false;
				FD[i].current_offset=0;
				FD[i].current_wblock=file.b_inode.iNode[freeinode].dp[0];
				FD[i].current_woffset=0;
				return i;
			}
		}
		FD_t tempfd;
		tempfd.valid=1;
		//tempfd.dir_no=cur_dir;
		short int freeinode = foundinodeno;

		FD[i].inode_no = freeinode;
		file.b_inode.iNode[freeinode].dp.resize(5,-1);
		file.b_inode.iNode[freeinode].dp[0]=file.sp.first_free_block;
		file.b_inode.iNode[freeinode].last_data_block = file.sp.first_free_block;

		tempfd.current_block=file.b_inode.iNode[freeinode].dp[0];
		tempfd.current_offset=0;

		file.sp.first_free_block=file.b[file.sp.first_free_block-3].next;
		file.sp.free_inode[freeinode]=false;

		tempfd.current_wblock=file.b_inode.iNode[freeinode].dp[0];
		tempfd.current_woffset=0;

		FD.push_back(tempfd);
		return (FD.size()-1);
	}

}

/////////////////////////////////////////////////////////////////////////////////////////
int my_read(int fd, char* buf, int count)
{
	int inodeno = FD[fd].inode_no; //jobhi inode we are working on
	int currentdatablockno = FD[fd].current_block;  //initialize to dp[0] wala block no
	int currentdatablockoffset = FD[fd].current_offset; //initially 0
	int indirectiontype = FD[fd].indirection_type; //initially 0
	int directoffset = FD[fd].directoffset; //initially 0
	int sipblockoffset = FD[fd].index_sip; //initially 0
	int dipblockoffset1 = FD[fd].index_dip1; //initially 0
	int dipblockoffset2 = FD[fd].index_dip2; //initially 0

	//if fd is not valid then return -1
	if(fd>=FD.size() || fd<0 || FD[fd].valid==0)
	{
		return -1;
	}
	//else read till count or eof
	int i;
	int bufpointer = 0;
	// cout<<"Reading in block : "<<currentdatablockno<<" offset : "<<currentdatablockoffset<<endl;
	for(i=0; i<count; i++)
	{
		buf[bufpointer++] = file.b[FD[fd].current_block-3].data[FD[fd].current_offset];
		if(buf[bufpointer-1]  == '\0')
			break;
		
		indirectiontype = getpointertonextentry(inodeno, 1, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);		
		
		FD[fd].current_block = currentdatablockno;
		FD[fd].current_offset = currentdatablockoffset;
		FD[fd].indirection_type = indirectiontype;
		FD[fd].directoffset = directoffset;
		FD[fd].index_sip = sipblockoffset;
		FD[fd].index_dip1 = dipblockoffset1;
		FD[fd].index_dip2 = dipblockoffset2;
		
		if(indirectiontype == -2)
			return -1;
		if(indirectiontype == -1){
			int freeblock = file.sp.first_free_block;
			file.sp.first_free_block = file.b[freeblock-3].next;

			if(directoffset < 4){
				file.b_inode.iNode[inodeno].dp[directoffset+1] = freeblock;
			}
			file.b_inode.iNode[inodeno].last_data_block = freeblock;

			indirectiontype = getpointertonextentry(inodeno, 1, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);

			FD[fd].current_block = currentdatablockno;
			FD[fd].current_offset = currentdatablockoffset;
			FD[fd].indirection_type = indirectiontype;
			FD[fd].directoffset = directoffset;
			FD[fd].index_sip = sipblockoffset;
			FD[fd].index_dip1 = dipblockoffset1;
			FD[fd].index_dip2 = dipblockoffset2;
		}
		
	}
	buf[bufpointer]='\0';
	return bufpointer;
}

////////////////////////////////////////////////////////////////////////////////////////
int my_write(int fd, char *buf, int count)
{
	int inodeno = FD[fd].inode_no; //jobhi inode we are working on
	int currentdatablockno = FD[fd].current_wblock;  //initialize to dp[0] wala block no
	int currentdatablockoffset = FD[fd].current_woffset; //initially 0
	int indirectiontype = FD[fd].windirection_type; //initially 0
	int directoffset = FD[fd].wdirectoffset; //initially 0
	int sipblockoffset = FD[fd].windex_sip; //initially 0
	int dipblockoffset1 = FD[fd].windex_dip1; //initially 0
	int dipblockoffset2 = FD[fd].windex_dip2; //initially 0

	//if fd is not valid then return -1
	if(fd>=FD.size() || fd<0 || FD[fd].valid==0)
	{
		return -1;
	}
	//else write till count
	int i;
	int bufpointer = 0;
	// cout<<"Writing in block : "<<currentdatablockno<<" offset : "<<currentdatablockoffset<<endl;
	for(i=0; i<count; i++){
		if(file.sp.block_size - currentdatablockoffset >= 1)
		{

			file.b[FD[fd].current_block-3].data[FD[fd].current_woffset] = buf[bufpointer++];
			// cout<<file.b[FD[fd].current_block-3].data[FD[fd].current_woffset]<<endl;
			if(buf[bufpointer-1] == '\0')
				break;
			indirectiontype = getpointertonextentry(inodeno, 1, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);		
		
			FD[fd].current_wblock = currentdatablockno;
			FD[fd].current_woffset = currentdatablockoffset;
			FD[fd].windirection_type = indirectiontype;
			FD[fd].wdirectoffset = directoffset;
			FD[fd].windex_sip = sipblockoffset;
			FD[fd].windex_dip1 = dipblockoffset1;
			FD[fd].windex_dip2 = dipblockoffset2;
			
			if(indirectiontype == -2)
				return -1;
		}
		else{
			int freeblock = file.sp.first_free_block;
			file.sp.first_free_block = file.b[freeblock-3].next;

			if(directoffset < 4){
				file.b_inode.iNode[inodeno].dp[directoffset+1] = freeblock;
			}
			file.b_inode.iNode[inodeno].last_data_block = freeblock;

			file.b[FD[fd].current_block-3].data[FD[fd].current_woffset] = buf[bufpointer++];
			if(buf[bufpointer-1] == '\0')
				break;
			indirectiontype = getpointertonextentry(inodeno, 1, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);
			
			FD[fd].current_wblock = currentdatablockno;
			FD[fd].current_woffset = currentdatablockoffset;
			FD[fd].windirection_type = indirectiontype;
			FD[fd].wdirectoffset = directoffset;
			FD[fd].windex_sip = sipblockoffset;
			FD[fd].windex_dip1 = dipblockoffset1;
			FD[fd].windex_dip2 = dipblockoffset2;

			if(indirectiontype == -2)
				return -1;

		}
	}
	return bufpointer;
}

/////////////////////////////////////////////////////////////////////////////////////////
int my_close(int fd)
{
	int inodeno = cur_dir; //jobhi inode we are working on
	int currentdatablockno = file.b_inode.iNode[cur_dir].dp[1];  //initialize to dp[0] wala block no
	int currentdatablockoffset = 0; //initially 0
	int indirectiontype = 0; //initially 0
	int directoffset = 0; //initially 0
	int sipblockoffset = 0; //initially 0
	int dipblockoffset1 = 0; //initially 0
	int dipblockoffset2 = 0; //initially 0

	//if fd is not valid then return -1
	if(fd>=FD.size() || fd<0 || FD[fd].valid==0)
	{
		return -1;
	}
	//else drop the entry
	FD[fd].valid = 0;
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////
int my_cat(int fd){
	int inodeno = FD[fd].inode_no; //jobhi inode we are working on
	int currentdatablockno = file.b_inode.iNode[FD[fd].inode_no].dp[0];  //initialize to dp[0] wala block no
	int currentdatablockoffset = 0; //initially 0
	int indirectiontype = 0; //initially 0
	int directoffset = 0; //initially 0
	int sipblockoffset = 0; //initially 0
	int dipblockoffset1 = 0; //initially 0
	int dipblockoffset2 = 0; //initially 0

	//if fd is not valid then return -1
	if(fd>=FD.size() || fd<0 || FD[fd].valid==0)
	{
		return -1;
	}
	// cout<<"Cat from block :"<<currentdatablockno<<" offset :"<<currentdatablockoffset<<endl;
	while(1){
		cout<<file.b[currentdatablockno-3].data[currentdatablockoffset];
		if(file.b[currentdatablockno-3].data[currentdatablockoffset]  == '\0')
			break;

		indirectiontype = getpointertonextentry(inodeno, 1, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);		
		
		
		if(indirectiontype == -2)
			return -1;
		if(indirectiontype == -1){
			int freeblock = file.sp.first_free_block;
			file.sp.first_free_block = file.b[freeblock-3].next;

			if(directoffset < 4){
				file.b_inode.iNode[inodeno].dp[directoffset+1] = freeblock;
			}
			file.b_inode.iNode[inodeno].last_data_block = freeblock;

			indirectiontype = getpointertonextentry(inodeno, 1, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);
			continue;
		}
	}
	cout<<endl;
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
int my_copy(int fd, int linuxfd, int flag){  // flag = 0 means from linuxfd to fd, flag = 1 means from fd to linuxfd
	if(flag == 0){

		if(fd>=FD.size() || fd<0 || FD[fd].valid==0)
		{
			return -1;
		}
		//read from location pointed to by buffer(count no of characters and store in fp to ahead
		char ch;
		int c;
		int bufpointer = 0;

		int inodeno = FD[fd].inode_no; //jobhi inode we are working on
		int currentdatablockno = FD[fd].current_wblock;  //initialize to dp[0] wala block no
		int currentdatablockoffset = FD[fd].current_woffset; //initially 0
		int indirectiontype = FD[fd].windirection_type; //initially 0
		int directoffset = FD[fd].wdirectoffset; //initially 0
		int sipblockoffset = FD[fd].windex_sip; //initially 0
		int dipblockoffset1 = FD[fd].windex_dip1; //initially 0
		int dipblockoffset2 = FD[fd].windex_dip2; //initially 0

		while(1){
			c = read(linuxfd, &ch, 1);
			bufpointer += c;
			if(c<1)
				break;

			if(file.sp.block_size - currentdatablockoffset >= 1)
			{

				file.b[FD[fd].current_block-3].data[FD[fd].current_woffset] = ch;
				// cout<<file.b[FD[fd].current_block-3].data[FD[fd].current_woffset]<<endl;
				// if(buf[bufpointer-1] == '\0')
					// break;
				indirectiontype = getpointertonextentry(inodeno, 1, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);		
			
				FD[fd].current_wblock = currentdatablockno;
				FD[fd].current_woffset = currentdatablockoffset;
				FD[fd].windirection_type = indirectiontype;
				FD[fd].wdirectoffset = directoffset;
				FD[fd].windex_sip = sipblockoffset;
				FD[fd].windex_dip1 = dipblockoffset1;
				FD[fd].windex_dip2 = dipblockoffset2;
				
				if(indirectiontype == -2)
					return -1;
			}
			else{
				int freeblock = file.sp.first_free_block;
				file.sp.first_free_block = file.b[freeblock-3].next;

				if(directoffset < 4){
					file.b_inode.iNode[inodeno].dp[directoffset+1] = freeblock;
				}
				file.b_inode.iNode[inodeno].last_data_block = freeblock;

				file.b[FD[fd].current_block-3].data[FD[fd].current_woffset] = ch;
				// if(buf[bufpointer-1] == '\0')
					// break;
				indirectiontype = getpointertonextentry(inodeno, 1, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);
				
				FD[fd].current_wblock = currentdatablockno;
				FD[fd].current_woffset = currentdatablockoffset;
				FD[fd].windirection_type = indirectiontype;
				FD[fd].wdirectoffset = directoffset;
				FD[fd].windex_sip = sipblockoffset;
				FD[fd].windex_dip1 = dipblockoffset1;
				FD[fd].windex_dip2 = dipblockoffset2;

				if(indirectiontype == -2)
					return -1;

			}
			
		}
		return bufpointer;
	}
	
	else if(flag == 1){
		// cout<<"HELLO"<<endl;
		if(fd>=FD.size() || fd<0 || FD[fd].valid==0)
		{
			return -1;
		}

		char ch;
		int c;
		int bufpointer = 0;

		int inodeno = FD[fd].inode_no; //jobhi inode we are working on
		int currentdatablockno = file.b_inode.iNode[FD[fd].inode_no].dp[0];  //initialize to dp[0] wala block no
		int currentdatablockoffset = 0; //initially 0
		int indirectiontype = 0; //initially 0
		int directoffset = 0; //initially 0
		int sipblockoffset = 0; //initially 0
		int dipblockoffset1 = 0; //initially 0
		int dipblockoffset2 = 0; //initially 0

		while(1){
			ch = file.b[currentdatablockno-3].data[currentdatablockoffset];
			// cout<<"ch :"<<ch<<endl;
			if(ch == '\0') break;
			// cout<<"Writing to file"<<endl;
			c = write(linuxfd, &ch, 1);
			bufpointer += 1;
			if(c<1)
				break;

			indirectiontype = getpointertonextentry(inodeno, 1, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);		
			
			
			if(indirectiontype == -2)
				return -1;
			if(indirectiontype == -1){
				int freeblock = file.sp.first_free_block;
				file.sp.first_free_block = file.b[freeblock-3].next;

				if(directoffset < 4){
					file.b_inode.iNode[inodeno].dp[directoffset+1] = freeblock;
				}
				file.b_inode.iNode[inodeno].last_data_block = freeblock;

				indirectiontype = getpointertonextentry(inodeno, 1, &currentdatablockno, &currentdatablockoffset, &indirectiontype, &directoffset, &sipblockoffset, &dipblockoffset1, &dipblockoffset2);
				continue;
			}
		}
		return bufpointer;
	}
	else
		return -1;
}
