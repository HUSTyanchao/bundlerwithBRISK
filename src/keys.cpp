
/* ��.key�ļ��ж�ȡ�����ӵ���Ŀ����ȡ�����ӵ�λ�á��߶ȡ�����(4 float),��DESCRIPTOR_LENGTHά�������ӡ�
	Ϊ����Image��.key�ļ���ȡ�����Ӳ�����ƥ�䲢prune���õ����ƥ��
*/
/* keys.cpp */
/* Class for SIFT keypoints, modified for BRISK */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <intrin.h>

#ifndef WIN32
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#include "keys.h"

#include "defines.h"

#ifdef __BUNDLER_DISTR__
#include "ANN/ANN.h"
#else
#include "ann_1.1_char/include/ANN/ANN.h"
#endif



int GetNumberOfKeysNormal(FILE *fp)
{
    int num, len;

    if (fscanf(fp, "%d %d", &num, &len) != 2) {
	printf("Invalid keypoint file.\n");
	return 0;
    }

#ifdef KEY_LIMIT
    num = MIN(num, 65536); // we'll store at most 65536 features per
                           // image
#endif /* KEY_LIMIT */

    return num;
}

int GetNumberOfKeysGzip(gzFile fp)
{
    int num, len;

    char header[256];
    gzgets(fp, header, 256);

    if (sscanf(header, "%d %d", &num, &len) != 2) {
	printf("Invalid keypoint file.\n");
	return 0;
    }

#ifdef KEY_LIMIT
    num = MIN(num, 65536); // we'll store at most 65536 features per
                           // image
#endif /* KEY_LIMIT */

    return num;
}

int GetNumberOfKeysBin(FILE *f)
{
    int num;
    fread(&num, sizeof(int), 1, f);
    
#ifdef KEY_LIMIT
    num = MIN(num, 65536); // we'll store at most 65536 features per
                           // image
#endif /* KEY_LIMIT */

    return num;
}

int GetNumberOfKeysBinGzip(gzFile gzf)
{
    int num;
    gzread(gzf, &num, sizeof(int));
    
#ifdef KEY_LIMIT
    num = MIN(num, 65536); // we'll store at most 65536 features per
                           // image
#endif /* KEY_LIMIT */

    return num;
}

/* Returns the number of keys in a file */
int GetNumberOfKeys(const char *filename)
{
    FILE *file;

    file = fopen (filename, "r");
    if (! file) {
        /* Try to open a gzipped keyfile */
        char buf[1024];
        sprintf(buf, "%s.gz", filename);
        gzFile gzf = gzopen(buf, "rb");

        if (gzf == NULL) {
            /* Try to open a .bin file */
            sprintf(buf, "%s.bin", filename);
            file = fopen(buf, "rb");
            
            if (file == NULL) {
                /* Try to open a gzipped .bin file */
                sprintf(buf, "%s.bin.gz", filename);
                gzf = gzopen(buf, "rb");
                
                if (gzf == NULL) {
                    printf("Could not open file: %s\n", filename);
                    return 0;
                } else {
                    int n = GetNumberOfKeysBinGzip(gzf);
                    gzclose(gzf);
                    return n;
                }
            } else {
                int n = GetNumberOfKeysBin(file);
                fclose(file);
                return n;
            }
        } else {
            int n = GetNumberOfKeysGzip(gzf);
            gzclose(gzf);
            return n;
        }
    } else {
        int n = GetNumberOfKeysNormal(file);
        fclose(file);
        return n;
    }
}

/* This reads a keypoint file from a given filename and returns the list
 * of keypoints. */
std::vector<KeypointWithDesc> ReadKeyFileWithDesc(const char *filename,
                                                  bool descriptor)
{
    FILE *file;

    file = fopen (filename, "r");
    if (! file) {
        /* Try to file a gzipped keyfile */
        char buf[1024];
        sprintf(buf, "%s.gz", filename);
        gzFile gzf = gzopen(buf, "rb");

        if (gzf == NULL) {
            /* Try to open a .bin file */
            sprintf(buf, "%s.bin", filename);
            file = fopen(buf, "rb");
            
            if (file == NULL) {
                /* Try to open a gzipped .bin file */
                sprintf(buf, "%s.bin.gz", filename);
                gzf = gzopen(buf, "rb");
                
                if (gzf == NULL) {
                    std::vector<KeypointWithDesc> empty;
                    printf("Could not open file: %s\n", filename);
                    return empty;
                } else {
                    std::vector<KeypointWithDesc> kps_desc = 
                        ReadKeysFastBinGzip(gzf, descriptor);
                    gzclose(gzf);
                    return kps_desc;
                }
            } else {
                std::vector<KeypointWithDesc> kps_desc = 
                    ReadKeysFastBin(file, descriptor);
                fclose(file);
                return kps_desc;
            }
        } else {
            std::vector<KeypointWithDesc> kps_desc = 
                ReadKeysFastGzip(gzf, descriptor);
            gzclose(gzf);
            return kps_desc;
        }
    } else {
        std::vector<KeypointWithDesc> kps_desc = ReadKeysFast(file, descriptor);
        fclose(file);
        return kps_desc;
    }
}

std::vector<Keypoint> ReadKeyFile(const char *filename)
{
    std::vector<KeypointWithDesc> kps_d = ReadKeyFileWithDesc(filename, false);

    std::vector<Keypoint> kps;
    int num_keys = (int) kps_d.size();
    kps.resize(num_keys);
    for (int i = 0; i < num_keys; i++) {
        kps[i].m_x = kps_d[i].m_x;
        kps[i].m_y = kps_d[i].m_y;
    }

    kps_d.clear();

    return kps;
}

/* This reads a keypoint file from a given filename and returns the list
 * of keypoints. */
std::vector<KeypointWithScaleRot> 
    ReadKeyFileWithScaleRot(const char *filename, bool descriptor)
{
    FILE *file;
    std::vector<KeypointWithDesc> kps;
    float *scale = NULL, *orient = NULL;

    file = fopen (filename, "r");
    if (! file) {
        /* Try to file a gzipped keyfile */
        char buf[1024];
        sprintf(buf, "%s.gz", filename);
        gzFile gzf = gzopen(buf, "rb");

        if (gzf == NULL) {
            /* Try to open a .bin file */
            sprintf(buf, "%s.bin", filename);
            file = fopen(buf, "rb");
            
            if (file == NULL) {
                /* Try to open a gzipped .bin file */
                sprintf(buf, "%s.bin.gz", filename);
                gzf = gzopen(buf, "rb");
                
                if (gzf == NULL) {
                    std::vector<KeypointWithScaleRot> empty;
                    printf("Could not open file: %s\n", filename);
                    return empty;
                } else {
                    kps = ReadKeysFastBinGzip(gzf, descriptor, &scale, &orient);
                    gzclose(gzf);
                }
            } else {
                kps = ReadKeysFastBin(file, descriptor, &scale, &orient);
                fclose(file);
            }
        } else {
            kps = ReadKeysFastGzip(gzf, descriptor, &scale, &orient);
            gzclose(gzf);
        }
    } else {
        kps = ReadKeysFast(file, descriptor, &scale, &orient);
        fclose(file);
    }

    std::vector<KeypointWithScaleRot> kps_w;
    int num_keys = (int) kps.size();
    kps_w.resize(num_keys);
    for (int i = 0; i < num_keys; i++) {
        kps_w[i].m_x = kps[i].m_x;
        kps_w[i].m_y = kps[i].m_y;
        kps_w[i].m_d = kps[i].m_d;
        kps_w[i].m_scale = scale[i];
        kps_w[i].m_orient = orient[i];
    }

    kps.clear();

    if (scale != NULL)
        delete [] scale;

	if (orient != NULL)
        delete [] orient;

    return kps_w;
}

static char *strchrn(char *str, int c, int n) {
    for (int i = 0; i < n; i++) {
		str = strchr(str, c) + 1;
		if (str == NULL) return NULL;
    }

    return str - 1;
}

/* Read keypoints from the given file pointer and return the list of
 * keypoints.  The file format starts with 2 integers giving the total
 * number of keypoints and the size of descriptor vector for each
 * keypoint (currently assumed to be DESCRIPTOR_LENGTH). Then each keypoint is
 * specified by 4 floating point numbers giving subpixel row and
 * column location, scale, and orientation (in radians from -PI to
 * PI).  Then the descriptor vector for each keypoint is given as a
 * list of integers in range [0,255]. */
std::vector<Keypoint> ReadKeys(FILE *fp, bool descriptor)
{
    int i, j, num, len, val;

    std::vector<Keypoint> kps;

    if (fscanf(fp, "%d %d", &num, &len) != 2) {
		printf("Invalid keypoint file beginning.");
		return kps;
    }

#ifdef KEY_LIMIT
    num = MIN(num, 65536); // we'll store at most 65536 features per
                           // image
#endif /* KEY_LIMIT */

    if (len != DESCRIPTOR_LENGTH) {
		printf("Keypoint descriptor length invalid (should be DESCRIPTOR_LENGTH).");
		return kps;
    }

    for (i = 0; i < num; i++) {
	/* Allocate memory for the keypoint. */
	unsigned char *d = new unsigned char[len];
	float x, y, scale, ori;

	if (fscanf(fp, "%f %f %f %f", &y, &x, &scale, &ori) != 4) {
	    printf("Invalid keypoint file format.");
	    return kps;
	}

	for (j = 0; j < len; j++) {
	    if (fscanf(fp, "%d", &val) != 1 || val < 0 || val > 255) {
			printf("Invalid keypoint file value.");
			return kps;
	    }
	    d[j] = (unsigned char) val;
	}

	if (descriptor) {
	    kps.push_back(KeypointWithDesc(x, y, d));
	} else {
	    delete [] d;
	    kps.push_back(Keypoint(x, y));
	}
    }

    return kps;
}

/* Read keys more quickly */
std::vector<KeypointWithDesc> ReadKeysFast(FILE *fp, bool descriptor,
                                           float **scales, float **orients)
{
    int i, j, num, len;

    std::vector<KeypointWithDesc> kps;

    if (fscanf(fp, "%d %d", &num, &len) != 2) {
		printf("Invalid keypoint file beginning.");
		return kps;
    }

#ifdef KEY_LIMIT
    num = MIN(num, 65536); // we'll store at most 65536 features per
                           // image
#endif /* KEY_LIMIT */

    if (len != DESCRIPTOR_LENGTH) {
		printf("Keypoint descriptor length invalid (should be DESCRIPTOR_LENGTH).");
		return kps;
    }

    kps.resize(num);

    if (num > 0 && scales != NULL) {
        *scales = new float[num];
    }

    if (num > 0 && orients != NULL) {
        *orients = new float[num];
    }

    for (i = 0; i < num; i++) {
	/* Allocate memory for the keypoint. */
	float x, y, scale, ori;

	if (fscanf(fp, "%f %f %f %f\n", &y, &x, &scale, &ori) != 4) {
	    printf("Invalid keypoint file format.");
	    return kps;
	}

        if (scales != NULL) {
            (*scales)[i] = scale;
        }
        
        if (orients != NULL) {
            (*orients)[i] = ori;
        }

	char buf[1024];

	/* Allocate memory for the keypoint. */
	unsigned char *d = NULL;
	
	if (descriptor)
	    d = new unsigned char[len];

	int start = 0;
	for (int line = 0; line < (DESCRIPTOR_LENGTH/20)+1; line++) {
	    fgets(buf, 1024, fp);

	    if (!descriptor) continue;

	    unsigned char p[20];
		//short int p[20];

		if (line < (DESCRIPTOR_LENGTH/20)) {
				sscanf(buf, 
		       "%hu %hu %hu %hu %hu %hu %hu %hu %hu %hu "
		       "%hu %hu %hu %hu %hu %hu %hu %hu %hu %hu", 
		       p+0, p+1, p+2, p+3, p+4, p+5, p+6, p+7, p+8, p+9, 
		       p+10, p+11, p+12, p+13, p+14, 
		       p+15, p+16, p+17, p+18, p+19);

			for (j = 0; j < 20; j++)
				d[start + j] = p[j];
			start += 20;
	    } 
		else {
			for (j = 0; j < (DESCRIPTOR_LENGTH %20); j++){
				sscanf(buf, "%hu", &p[j]);
				d[start + j] = p[j];
				/*sscanf(buf,"%hu %hu %hu %hu",p+0, p+1, p+2, p+3);*/
			}
	    }
	}

    // kps.push_back(KeypointWithDesc(x, y, d));
    kps[i] = KeypointWithDesc(x, y, d);
    }

    return kps;
}

std::vector<KeypointWithDesc> ReadKeysFastGzip(gzFile fp, bool descriptor,
                                               float **scales, float **orients)
{
    int i, j, num, len;

    std::vector<KeypointWithDesc> kps;
    char header[256];
    gzgets(fp, header, 256);

    if (sscanf(header, "%d %d", &num, &len) != 2) {
	printf("Invalid keypoint file.\n");
	return kps;
    }

#ifdef KEY_LIMIT
    num = MIN(num, 65536); // we'll store at most 65536 features per
                           // image
#endif /* KEY_LIMIT */

    if (len != DESCRIPTOR_LENGTH) {
	printf("Keypoint descriptor length invalid (should be DESCRIPTOR_LENGTH).");
	return kps;
    }

    kps.resize(num);

    if (num > 0 && scales != NULL) {
        *scales = new float[num];
    }

    if (num > 0 && orients != NULL) {
        *orients = new float[num];
    }

    for (i = 0; i < num; i++) {
	/* Allocate memory for the keypoint. */
	float x, y, scale, ori;
        char buf[1024];
        gzgets(fp, buf, 1024);

	if (sscanf(buf, "%f %f %f %f\n", &y, &x, &scale, &ori) != 4) {
	    printf("Invalid keypoint file format.");
	    return kps;
	}

        if (scales != NULL) {
            (*scales)[i] = scale;
        }
        
        if (orients != NULL) {
            (*orients)[i] = ori;
        }

	/* Allocate memory for the keypoint. */
	unsigned char *d = NULL;
	
	if (descriptor)
	    d = new unsigned char[len];

	int start = 0;
	for (int line = 0; line < (DESCRIPTOR_LENGTH/20 + 1); line++) {
	    gzgets(fp, buf, 1024);

	    if (!descriptor) continue;

	    //short int p[20];
		unsigned char p[20];

		if (line < (DESCRIPTOR_LENGTH/20)) {
		sscanf(buf, 
		       "%hu %hu %hu %hu %hu %hu %hu %hu %hu %hu "
		       "%hu %hu %hu %hu %hu %hu %hu %hu %hu %hu", 
		       p+0, p+1, p+2, p+3, p+4, p+5, p+6, p+7, p+8, p+9, 
		       p+10, p+11, p+12, p+13, p+14, 
		       p+15, p+16, p+17, p+18, p+19);

		for (j = 0; j < 20; j++)
		    d[start + j] = p[j];

		start += 20;
	    } else {
			for (j = 0; j < (DESCRIPTOR_LENGTH % 20); j++){
				sscanf(buf, "%hu", &p[j]);
				d[start + j] = p[j];
				/*sscanf(buf,"%hu %hu %hu %hu",p+0, p+1, p+2, p+3);*/
			}
	    }
	}

        // kps.push_back(KeypointWithDesc(x, y, d));
        kps[i] = KeypointWithDesc(x, y, d);
    }

    return kps;
}

/* Read keys from binary file */
std::vector<KeypointWithDesc> ReadKeysFastBin(FILE *fp, bool descriptor,
                                              float **scales, 
                                              float **orients)
{
    int num_keys;
    fread(&num_keys, sizeof(int), 1, fp);

    std::vector<KeypointWithDesc> keys;
    keys.resize(num_keys);

    keypt_t *info;
    unsigned char *d;

    info = new keypt_t[num_keys];
    
    fread(info, sizeof(keypt_t), num_keys, fp);

    if (scales != NULL)
        *scales = new float[num_keys];
    
    if (orients != NULL)
        *orients = new float[num_keys];

    for (int i = 0; i < num_keys; i++) {
        keys[i].m_x = info[i].x;
        keys[i].m_y = info[i].y;
        
        if (scales != NULL)
            (*scales)[i] = info[i].scale;
        if (orients != NULL)
            (*orients)[i] = info[i].orient;
    }

    delete [] info;

    if (!descriptor)
        return keys;
    
    d = new unsigned char [DESCRIPTOR_LENGTH * num_keys];

    fread(d, sizeof(unsigned char), DESCRIPTOR_LENGTH * num_keys, fp);
    
    for (int i = 0; i < num_keys; i++) {
        keys[i].m_d = d + DESCRIPTOR_LENGTH * i;
    }

    return keys;
}

/* Read keys from gzipped binary file */
std::vector<KeypointWithDesc> ReadKeysFastBinGzip(gzFile fp, bool descriptor,
                                                  float **scales, 
                                                  float **orients)
{
    int num_keys;
    gzread(fp, &num_keys, sizeof(int));

    std::vector<KeypointWithDesc> keys;
    keys.resize(num_keys);

    keypt_t *info;
    unsigned char *d;

    info = new keypt_t[num_keys];
    
    gzread(fp, info, sizeof(keypt_t) * num_keys);

    if (scales != NULL)
        *scales = new float[num_keys];
    
    if (orients != NULL)
        *orients = new float[num_keys];

    for (int i = 0; i < num_keys; i++) {
        keys[i].m_x = info[i].x;
        keys[i].m_y = info[i].y;
        
        if (scales != NULL)
            (*scales)[i] = info[i].scale;
        if (orients != NULL)
            (*orients)[i] = info[i].orient;
    }

    delete [] info;

    if (!descriptor)
        return keys;
    
    d = new unsigned char [DESCRIPTOR_LENGTH * num_keys];

    gzread(fp, d, sizeof(unsigned char) * DESCRIPTOR_LENGTH * num_keys);
    
    for (int i = 0; i < num_keys; i++) {
        keys[i].m_d = d + DESCRIPTOR_LENGTH * i;
    }

    return keys;
}

//��ʹ�ø÷���ƥ������������ӡ�
#if 0
ann_1_1_char::ANNkd_tree
    *CreateSearchTreeChar(const std::vector<KeypointWithDesc> &k) 
{
    /* Create a new array of points */
    int num_pts = (int) k.size();

	int dim = DESCRIPTOR_LENGTH;

    ann_1_1_char::ANNpointArray pts = ann_1_1_char::annAllocPts(num_pts, dim);

    int offset = 0;
     
    for (int i = 0; i < num_pts; i++) {
        int j;
        
		for (j = 0; j < DESCRIPTOR_LENGTH; j++)
            pts[i][j+offset] = k[i].m_d[j];
    }
    
    /* Create a search tree for k2 */
    ann_1_1_char::ANNkd_tree *tree = 
        new ann_1_1_char::ANNkd_tree(pts, num_pts, dim, 4);

    // ann_1_1_char::annDeallocPts(pts);

    return tree;
}


//registeredΪ1ʱ��ֻƥ��k2��m_extra>=d�ĵ�,Ϊ0��ȫ��ƥ��
/* Compute likely matches between two sets of keypoints */
std::vector<KeypointMatch> MatchKeys(const std::vector<KeypointWithDesc> &k1, 
				     const std::vector<KeypointWithDesc> &k2, 
				     bool registered, double ratio) 
{
    ann_1_1_char::annMaxPtsVisit(200);

    int num_pts = 0;
    std::vector<KeypointMatch> matches;

    int *registered_idxs = NULL;
	
    if (!registered) {
		num_pts = (int) k2.size();
    } 
	//registeredΪ1ʱ��registered_idxs����k2����Щm_extra>=0�ĵ��������
	else {
		registered_idxs = new int[(int) k2.size()];
		for (int i = 0; i < (int) k2.size(); i++) {
			if (k2[i].m_extra >= 0) {
				registered_idxs[num_pts] = i;
				num_pts++;
			}
		}
    }

    /* Create a new array of points */
	ann_1_1_char::ANNpointArray pts = ann_1_1_char::annAllocPts(num_pts, DESCRIPTOR_LENGTH);

	//registerΪ0ʱ��pts����k2�������ӣ�ÿһ����һ��������
    if (!registered) {
		for (int i = 0; i < num_pts; i++) {
			int j;

			for (j = 0; j < DESCRIPTOR_LENGTH; j++) {
				pts[i][j] = k2[i].m_d[j];
		    }
		}
    } 
	//registeredΪ1ʱ��pts����k2��m_extra>=0�ĵ��������
	else {
		for (int i = 0; i < num_pts; i++) {
			int j;
			int idx = registered_idxs[i];

			for (j = 0; j < DESCRIPTOR_LENGTH; j++) {
				pts[i][j] = k2[idx].m_d[j];
		    }
		}	
    }
    
    // clock_t start = clock();
    /* Create a search tree for k2 */
	ann_1_1_char::ANNkd_tree *tree = new ann_1_1_char::ANNkd_tree(pts, num_pts, DESCRIPTOR_LENGTH, 4);
    // clock_t end = clock();
    
    // printf("Building tree took %0.3fs\n", 
    //        (end - start) / ((double) CLOCKS_PER_SEC));

    /* Now do the search */
	ann_1_1_char::ANNpoint query = ann_1_1_char::annAllocPt(DESCRIPTOR_LENGTH);
    // start = clock();
    for (int i = 0; i < (int) k1.size(); i++) {
		int j;

		for (j = 0; j < DESCRIPTOR_LENGTH; j++) {
			query[j] = k1[i].m_d[j];
		}

		ann_1_1_char::ANNidx nn_idx[2];
		ann_1_1_char::ANNdist dist[2];

		tree->annkPriSearch(query, 2, nn_idx, dist, 0.0);

		if (sqrt(((double) dist[0]) / ((double) dist[1])) <= ratio) {
			//registeredΪ0ʱ��matches����k1 k2ƥ�����������Ŷ�
			if (!registered) {
				matches.push_back(KeypointMatch(i, nn_idx[0]));
			} 
			//Ϊ1ʱ��rnn_idx[0]Ϊ��ǰpts����k1 iƥ��ģ�registered_idxs[nn_idx[0]]k2���������
			//����ʵ����Ҳ����matches����k1 k2ƥ�����������Ŷ�
			else {
				KeypointMatch match = KeypointMatch(i, registered_idxs[nn_idx[0]]);
				matches.push_back(match);
			}
		}
    }
    // end = clock();
    // printf("Searching tree took %0.3fs\n",
    //        (end - start) / ((double) CLOCKS_PER_SEC));

    int num_matches = (int) matches.size();

    printf("[MatchKeys] Found %d matches\n", num_matches);

    /* Cleanup */
    ann_1_1_char::annDeallocPts(pts);
    ann_1_1_char::annDeallocPt(query);

    delete tree;

    return matches;
}


//����������ڵľ���(scores)
/* Compute likely matches between two sets of keypoints */
std::vector<KeypointMatchWithScore> 
    MatchKeysWithScore(const std::vector<KeypointWithDesc> &k1, 
                       const std::vector<KeypointWithDesc> &k2,
                       bool registered, 
                       double ratio)
{
    ann_1_1_char::annMaxPtsVisit(200);

    int num_pts = 0;
    std::vector<KeypointMatchWithScore> matches;

    int *registered_idxs = NULL;

    if (!registered) {
	num_pts = (int) k2.size();
    } else {
	registered_idxs = new int[(int) k2.size()];
	for (int i = 0; i < (int) k2.size(); i++) {
	    if (k2[i].m_extra >= 0) {
		registered_idxs[num_pts] = i;
		num_pts++;
	    }
	}
    }

    /* Create a new array of points */
	ann_1_1_char::ANNpointArray pts = ann_1_1_char::annAllocPts(num_pts, DESCRIPTOR_LENGTH);

    if (!registered) {
	for (int i = 0; i < num_pts; i++) {
	    int j;

		for (j = 0; j < DESCRIPTOR_LENGTH; j++) {
		pts[i][j] = k2[i].m_d[j];
	    }
	}
    } else {
	for (int i = 0; i < num_pts; i++) {
	    int j;
	    int idx = registered_idxs[i];

		for (j = 0; j < DESCRIPTOR_LENGTH; j++) {
		pts[i][j] = k2[idx].m_d[j];
	    }
	}	
    }
    
    // clock_t start = clock();
    /* Create a search tree for k2 */
	ann_1_1_char::ANNkd_tree *tree = new ann_1_1_char::ANNkd_tree(pts, num_pts, DESCRIPTOR_LENGTH, 4);
    // clock_t end = clock();
    
    // printf("Building tree took %0.3fs\n", 
    //        (end - start) / ((double) CLOCKS_PER_SEC));

    /* Now do the search */
	ann_1_1_char::ANNpoint query = ann_1_1_char::annAllocPt(DESCRIPTOR_LENGTH);
    // start = clock();
    for (int i = 0; i < (int) k1.size(); i++) {
	int j;

	for (j = 0; j < DESCRIPTOR_LENGTH; j++) {
	    query[j] = k1[i].m_d[j];
	}

	ann_1_1_char::ANNidx nn_idx[2];
	ann_1_1_char::ANNdist dist[2];

	tree->annkPriSearch(query, 2, nn_idx, dist, 0.0);

	if (sqrt(((double) dist[0]) / ((double) dist[1])) <= ratio) {
	    if (!registered) {
                KeypointMatchWithScore match = 
                    KeypointMatchWithScore(i, nn_idx[0], (float) dist[0]);
                matches.push_back(match);
	    } else {
		KeypointMatchWithScore match = 
		    KeypointMatchWithScore(i, registered_idxs[nn_idx[0]], 
                                           (float) dist[0]);
		matches.push_back(match);
	    }
	}
    }
    // end = clock();
    // printf("Searching tree took %0.3fs\n",
    //        (end - start) / ((double) CLOCKS_PER_SEC));

    int num_matches = (int) matches.size();

    printf("[MatchKeysWithScore] Found %d matches\n", num_matches);

    /* Cleanup */
    ann_1_1_char::annDeallocPts(pts);
    ann_1_1_char::annDeallocPt(query);

    delete tree;

    return matches;    
}


//һ�Զ��ƥ�䣬�������ƥ��
/* Prune matches so that they are 1:1 */
std::vector<KeypointMatchWithScore> 
    PruneMatchesWithScore(const std::vector<KeypointMatchWithScore> &matches)
{
#ifndef WIN32
    __gnu_cxx::hash_map<int, float> key_hash;
    __gnu_cxx::hash_map<int, int> map;
#else
    stdext::hash_map<int, float> key_hash;
    stdext::hash_map<int, int> map;
#endif

    int num_matches = (int) matches.size();
    
    for (int i = 0; i < num_matches; i++) {
        int idx1 = matches[i].m_idx1;
        int idx2 = matches[i].m_idx2;

        if (key_hash.find(idx2) == key_hash.end()) {
            /* Insert the new element */
            key_hash[idx2] = matches[i].m_score;
            map[idx2] = idx1;
        } else {
            float old = key_hash[idx2];
            if (old > matches[i].m_score) {
                /* Replace the old entry */
                key_hash[idx2] = matches[i].m_score;
                map[idx2] = idx1;
            }
        }
    }

    std::vector<KeypointMatchWithScore> matches_new;
    /* Now go through the list again, building a new list */
    for (int i = 0; i < num_matches; i++) {
        int idx1 = matches[i].m_idx1;
        int idx2 = matches[i].m_idx2;

        if (map[idx2] == idx1) {
            matches_new.push_back(KeypointMatchWithScore(idx1, idx2, 
                                                         key_hash[idx2]));
        }
    }

    return matches_new;
}


//�������û��ʹ��
/* Compute likely matches between two sets of keypoints */
std::vector<KeypointMatch>
    MatchKeysExhaustive(const std::vector<KeypointWithDesc> &k1, 
                        const std::vector<KeypointWithDesc> &k2, 
                        bool registered, double ratio) 
{
    int num_pts = 0;
    std::vector<KeypointMatch> matches;

    int *registered_idxs = NULL;

    if (!registered) {
	num_pts = (int) k2.size();
    } else {
	registered_idxs = new int[(int) k2.size()];
	for (int i = 0; i < (int) k2.size(); i++) {
	    if (k2[i].m_extra >= 0) {
		registered_idxs[num_pts] = i;
		num_pts++;
	    }
	}
    }

    /* Create a new array of points */
	ann_1_1_char::ANNpointArray pts = ann_1_1_char::annAllocPts(num_pts, DESCRIPTOR_LENGTH);

    if (!registered) {
	for (int i = 0; i < num_pts; i++) {
	    int j;

	    for (j = 0; j < DESCRIPTOR_LENGTH; j++) {
		pts[i][j] = k2[i].m_d[j];
	    }
	}
    } else {
	for (int i = 0; i < num_pts; i++) {
	    int j;
	    int idx = registered_idxs[i];

		for (j = 0; j < DESCRIPTOR_LENGTH; j++) {
		pts[i][j] = k2[idx].m_d[j];
	    }
	}	
    }
    
    // clock_t start = clock();
    /* Create a search tree for k2 */
    ann_1_1_char::ANNkd_tree *tree = 
        new ann_1_1_char::ANNkd_tree(pts, num_pts, DESCRIPTOR_LENGTH, 4);
    // clock_t end = clock();
    
    // printf("Building tree took %0.3fs\n", 
    //        (end - start) / ((double) CLOCKS_PER_SEC));

    /* Now do the search */
    ann_1_1_char::ANNpoint query = ann_1_1_char::annAllocPt(DESCRIPTOR_LENGTH);
    // start = clock();
    for (int i = 0; i < (int) k1.size(); i++) {
	int j;

	for (j = 0; j < DESCRIPTOR_LENGTH; j++) {
	    query[j] = k1[i].m_d[j];
	}

	ann_1_1_char::ANNidx nn_idx[2];
	ann_1_1_char::ANNdist dist[2];

	tree->annkSearch(query, 2, nn_idx, dist, 0.0);

	if (sqrt(((double) dist[0]) / ((double) dist[1])) <= ratio) {
	    if (!registered) {
		matches.push_back(KeypointMatch(i, nn_idx[0]));
	    } else {
		KeypointMatch match = 
		    KeypointMatch(i, registered_idxs[nn_idx[0]]);
		matches.push_back(match);
	    }
	}
    }
    // end = clock();
    // printf("Searching tree took %0.3fs\n",
    //        (end - start) / ((double) CLOCKS_PER_SEC));

    int num_matches = (int) matches.size();

    printf("[MatchKeys] Found %d matches\n", num_matches);

    /* Cleanup */
    ann_1_1_char::annDeallocPts(pts);
    ann_1_1_char::annDeallocPt(query);

    delete tree;

    return matches;
}
#endif

//����ƥ�䷽ʽ����k2���ҵ�k1�����ƥ�䣬hamming����
// k1��ѯ�����㣬k2���ݿ�������
int BruteForceMatch(const KeypointWithDesc& k1,
	const std::vector<KeypointWithDesc>&k2, int*dist)
{
	unsigned int distance[3] = { DESCRIPTOR_LENGTH * 8, DESCRIPTOR_LENGTH * 8, DESCRIPTOR_LENGTH * 8 };
	//��С���룬 ��С���룬��ǰ����
	unsigned int index[3] = { 0 };
	unsigned short temp = 0;
	for (int i = 0; i < k2.size(); ++i)
	{
		distance[2] = 0;
		for (int j = 0; j < DESCRIPTOR_LENGTH/2; ++j)
		{
			//�����������
			temp = k1.m_d[j+1] ^ k2[i].m_d[j+1];
			temp = (temp << 8) | (k1.m_d[j] ^ k2[i].m_d[j]);
			distance[2] += __popcnt16(temp);
		}
		if (distance[2] < distance[1]){
			distance[1] = distance[2];
			index[1] = i;
			if (distance[1] <= distance[0]){
				distance[2] = distance[0];
				distance[0] = distance[1];
				distance[1] = distance[2];

				index[2] = index[0];
				index[0] = index[1];
				index[1] = index[2];
			}
		}
	}
	dist[0] = distance[0];
	dist[1] = distance[1];
	return index[0];
}

//registeredΪ1ʱ��ֻƥ��k2��m_extra>=d�ĵ�,Ϊ0��ȫ��ƥ��
/* Compute likely matches between two sets of keypoints */
std::vector<KeypointMatch> MatchKeys(const std::vector<KeypointWithDesc> &k1,
	const std::vector<KeypointWithDesc> &k2,
	bool registered, double ratio)
{
	int num_pts = 0;
	std::vector<KeypointMatch> matches;
	std::vector<KeypointWithDesc> pts;

	int *registered_idxs = NULL;

	//registerΪ0ʱ��pts����k2�������ӣ�ÿһ����һ��������
	if (!registered) {
		pts = k2;
		num_pts = (int)k2.size();
	}
	//registeredΪ1ʱ��registered_idxs����k2����Щm_extra>=0�ĵ��������
	else {
		registered_idxs = new int[(int)k2.size()];
		for (int i = 0; i < (int)k2.size(); i++) {
			if (k2[i].m_extra >= 0) {
				registered_idxs[num_pts] = i;
				pts.push_back(k2[i]);
				num_pts++;
			}
		}
	}


	for (int i = 0; i < (int)k1.size(); i++) 
	{
		int dist[2];
		int index = BruteForceMatch(k1[i], pts, dist);
		
		if (sqrt(((double)dist[0]) / ((double)dist[1])) <= ratio) {
			//registeredΪ0ʱ��matches����k1 k2ƥ�����������Ŷ�
			if (!registered) {
				matches.push_back(KeypointMatch(i, index));
			}
			//Ϊ1ʱ��rnn_idx[0]Ϊ��ǰpts����k1 iƥ��ģ�registered_idxs[nn_idx[0]]k2���������
			//����ʵ����Ҳ����matches����k1 k2ƥ�����������Ŷ�
			else {
				KeypointMatch match = KeypointMatch(i, registered_idxs[index]);
				matches.push_back(match);
			}
		}
	}


	int num_matches = (int)matches.size();
	printf("[MatchKeys] Found %d matches\n", num_matches);

	if (registered_idxs != NULL) delete registered_idxs;

	return matches;
}


//����������ڵľ���(scores)
/* Compute likely matches between two sets of keypoints */
std::vector<KeypointMatchWithScore>
MatchKeysWithScore(const std::vector<KeypointWithDesc> &k1,
const std::vector<KeypointWithDesc> &k2,
bool registered,
double ratio)
{
	int num_pts = 0;
	std::vector<KeypointMatchWithScore> matches;
	std::vector<KeypointWithDesc> pts;
	int *registered_idxs = NULL;

	//registerΪ0ʱ��pts����k2�������ӣ�ÿһ����һ��������
	if (!registered) {
		pts = k2;
		num_pts = (int)k2.size();
	}
	//registeredΪ1ʱ��registered_idxs����k2����Щm_extra>=0�ĵ��������
	else {
		registered_idxs = new int[(int)k2.size()];
		for (int i = 0; i < (int)k2.size(); i++) {
			if (k2[i].m_extra >= 0) {
				registered_idxs[num_pts] = i;
				pts.push_back(k2[i]);
				num_pts++;
			}
		}
	}

	for (int i = 0; i < (int)k1.size(); i++) {

		int dist[2];
		int index = BruteForceMatch(k1[i], pts, dist);

		if (sqrt(((double)dist[0]) / ((double)dist[1])) <= ratio) {
			if (!registered) {
				KeypointMatchWithScore match =
					KeypointMatchWithScore(i, index, (float)dist[0]);
				matches.push_back(match);
			}
			else {
				KeypointMatchWithScore match =
					KeypointMatchWithScore(i, registered_idxs[index],(float)dist[0]);
				matches.push_back(match);
			}
		}
	}

	int num_matches = (int)matches.size();
	printf("[MatchKeysWithScore] Found %d matches\n", num_matches);

	if (registered_idxs != NULL) delete registered_idxs;

	return matches;
}


//һ�Զ��ƥ�䣬�������ƥ��
/* Prune matches so that they are 1:1 */
std::vector<KeypointMatchWithScore>
PruneMatchesWithScore(const std::vector<KeypointMatchWithScore> &matches)
{
#ifndef WIN32
	__gnu_cxx::hash_map<int, float> key_hash;
	__gnu_cxx::hash_map<int, int> map;
#else
	stdext::hash_map<int, float> key_hash;
	stdext::hash_map<int, int> map;
#endif

	int num_matches = (int)matches.size();

	for (int i = 0; i < num_matches; i++) {
		int idx1 = matches[i].m_idx1;
		int idx2 = matches[i].m_idx2;

		if (key_hash.find(idx2) == key_hash.end()) {
			/* Insert the new element */
			key_hash[idx2] = matches[i].m_score;
			map[idx2] = idx1;
		}
		else {
			float old = key_hash[idx2];
			if (old > matches[i].m_score) {
				/* Replace the old entry */
				key_hash[idx2] = matches[i].m_score;
				map[idx2] = idx1;
			}
		}
	}

	std::vector<KeypointMatchWithScore> matches_new;
	/* Now go through the list again, building a new list */
	for (int i = 0; i < num_matches; i++) {
		int idx1 = matches[i].m_idx1;
		int idx2 = matches[i].m_idx2;

		if (map[idx2] == idx1) {
			matches_new.push_back(KeypointMatchWithScore(idx1, idx2,
				key_hash[idx2]));
		}
	}

	return matches_new;
}