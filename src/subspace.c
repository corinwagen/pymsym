//
//  subspace.c
//  libmsym
//
//  Created by Marcus Johansson on 28/05/15.
//  Copyright (c) 2014 Marcus Johansson.
//
//  Distributed under the MIT License ( See LICENSE file or copy at http://opensource.org/licenses/MIT )
//

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>

#include "msym.h"
#include "linalg.h"
#include "subspace.h"
#include "permutation.h"
#include "rsh.h"

#include "debug.h"

#define SQR(x) ((x)*(x))

msym_error_t generateRepresentations(int n, int sopsl, msym_symmetry_operation_t sops[sopsl], int lmax, rsh_representations_t *lrs);


msym_error_t generateRepresentations(int n, int sopsl, msym_symmetry_operation_t sops[sopsl], int lmax, rsh_representations_t *lrs){
    msym_error_t ret = MSYM_SUCCESS;
    for(int l = 0;l <= lmax;l++){
        int d = 2*l+1;
        lrs[l].d = d;
        lrs[l].t = malloc(sizeof(double[n][d][d]));
    }
    
    if(MSYM_SUCCESS != (ret = generateRSHRepresentations(sopsl, sops, lmax, lrs))) goto err;
    
    return ret;
err:
    for(int l = 0;l <= lmax;l++){
        free(lrs[l].t);
        lrs[l].t = NULL;
        lrs[l].d = 0;
    }
    return ret;
    
}

msym_error_t findSplittingFieldSubgroup(msym_point_group_t *pg, int irrep, int sgl, const msym_subgroup_t sg[sgl], msym_thresholds_t *thresholds, const msym_subgroup_t **osg){
    msym_error_t ret = MSYM_SUCCESS;
    *osg = NULL;
    msym_character_table_t *ct = pg->ct;
    
    switch(ct->s[irrep].d){
        case 2 : { //2-dimensional
            switch(pg->type){
                case MSYM_POINT_GROUP_TYPE_Cnv :
                case MSYM_POINT_GROUP_TYPE_Td  : {
                    for(int i = 0;i < sgl;i++){
                        if(sg[i].type == MSYM_POINT_GROUP_TYPE_Cs){
                            *osg = &sg[i];
                            break;
                        }
                    }
                    break;
                }
                case MSYM_POINT_GROUP_TYPE_Dn  :
                case MSYM_POINT_GROUP_TYPE_Dnd :
                case MSYM_POINT_GROUP_TYPE_Dnh :
                case MSYM_POINT_GROUP_TYPE_O   :
                case MSYM_POINT_GROUP_TYPE_Oh  : {
                    for(int i = 0;i < sgl;i++){
                        if(sg[i].type == MSYM_POINT_GROUP_TYPE_Cn && sg[i].n == 2){
                            int h = 0;
                            for(int j = 0;j < sg[i].order;j++){
                                msym_symmetry_operation_t *sop = sg[i].sops[j];
                                if(sop->type == PROPER_ROTATION && sop->order == 2 && sop->orientation == HORIZONTAL){
                                    h = 1;
                                    break;
                                }
                            }
                            if(!h){
                                *osg = &sg[i];
                                break;
                            }
                        }
                    }
                    break;
                }
                case MSYM_POINT_GROUP_TYPE_Cn  :
                case MSYM_POINT_GROUP_TYPE_Cnh :
                case MSYM_POINT_GROUP_TYPE_T   :
                case MSYM_POINT_GROUP_TYPE_Th  : {
                    ret = MSYM_SUBSPACE_ERROR;
                    msymSetErrorDetails("Cannot construct splitting field, point group %s has complex characters in symmetry species %s",pg->name, ct->s[irrep].name);
                    goto err;
                }
                default: break;
                    
            }
            break;
        }
        case 3 :{ //3-dimensional
            switch(pg->type){
                case MSYM_POINT_GROUP_TYPE_T  :
                case MSYM_POINT_GROUP_TYPE_Td :
                case MSYM_POINT_GROUP_TYPE_Th :
                case MSYM_POINT_GROUP_TYPE_O  :
                case MSYM_POINT_GROUP_TYPE_Oh :
                case MSYM_POINT_GROUP_TYPE_I  :
                case MSYM_POINT_GROUP_TYPE_Ih : {
                    for(int i = 0;i < sgl;i++){
                        if(sg[i].type == MSYM_POINT_GROUP_TYPE_Dn && sg[i].n == 2){
                            int h = 1;
                            for(int j = 0; j < sg[i].order; j++){
                                msym_symmetry_operation_t *sop = sg[i].sops[j];
                                if(!(sop->orientation == HORIZONTAL || sop->orientation == NONE)){
                                    h = 0;
                                    break;
                                }
                            }
                            if(h){
                                *osg = &sg[i];
                                break;
                            }
                        }
                    }
                }
                default: break;
            }
            break;
        }
        case 4 : { //4-dimensional
            if(!(MSYM_POINT_GROUP_TYPE_I == pg->type || MSYM_POINT_GROUP_TYPE_Ih == pg->type)){
                ret = MSYM_SUBSPACE_ERROR;
                msymSetErrorDetails("Invalid irrep dimension (4) when getting splitting field subgroup for non icosahedral point group %s", pg->name);
                goto err;
            }
            
            for(int i = 0;i < sgl;i++){
                if(sg[i].type == MSYM_POINT_GROUP_TYPE_Dn && sg[i].n == 2){
                    *osg = &sg[i];
                    break;
                }
            }
            
            break;
        }
        case 5 : { //5-dimensional
            if(!(MSYM_POINT_GROUP_TYPE_I == pg->type || MSYM_POINT_GROUP_TYPE_Ih == pg->type)){
                ret = MSYM_SUBSPACE_ERROR;
                msymSetErrorDetails("Invalid irrep dimension (5) when getting splitting field subgroup for non icosahedral point group %s", pg->name);
                goto err;
            }
            
            for(int i = 0;i < sgl;i++){
                if(sg[i].type == MSYM_POINT_GROUP_TYPE_Dn && sg[i].n == 5){
                    *osg = &sg[i];
                }
            }
            
            break;
        }
        default : break;
    }
    
    if(*osg == NULL){
        ret = MSYM_SUBSPACE_ERROR;
        msymSetErrorDetails("Could not find splitting filed subgroup in dimension %d for point group %s symmetry species %s",ct->s[irrep].d, pg->name, ct->s[irrep].name);
    }
err:
    return ret;
    
}

#define PHI 1.618033988749894848204586834

msym_error_t getSplittingFieldCharacters(msym_point_group_t *pg, const msym_subgroup_t *sg, double (*c)[pg->order], int *cd){
    msym_error_t ret = MSYM_SUCCESS;
    int e = 0;
    
    for(int i = 0;i < 5;i++){cd[i] = 1;}
    if((sg->type == MSYM_POINT_GROUP_TYPE_Cs) || (sg->type == MSYM_POINT_GROUP_TYPE_Cn && sg->n == 2)){
        int faxis = 0;
        memset(c, 0, sizeof(double[pg->order]));
        for(int s = 0;s < pg->order && !(e && faxis);s++){
            for(int i = 0;i < sg->order;i++){
                if(&pg->sops[s] != sg->sops[i]) continue;
                if(pg->sops[s].type == IDENTITY){
                    e = 1;
                    c[0][s] = c[1][s] = 1;
                    if(faxis) break;
                } else {
                    faxis = 1;
                    c[0][s] = 1;
                    c[1][s] = -1;
                    if(e) break;
                }
            }
        }
        
    } else if(sg->type == MSYM_POINT_GROUP_TYPE_Dn && sg->n == 2){
        int index = 0;
        double d2c[4][3] = {
            [0] = { 1, -1, -1},
            [1] = {-1, -1,  1},
            [2] = {-1,  1, -1},
            [3] = { 1,  1,  1}
        };
        memset(c, 0, sizeof(double[3][pg->order]));
        for(int s = 0;s < pg->order && !((index == 3) && e);s++){
            for(int i = 0;i < sg->order;i++){
                if(&pg->sops[s] != sg->sops[i]) continue;
                if(index == 3 && e) break;
                if(pg->sops[s].type == IDENTITY){
                    e = 1;
                    c[0][s] = c[1][s] = c[2][s] = c[3][s] = 1;
                    if(index == 3) break;
                } else {
                    c[0][s] = d2c[0][index];
                    c[1][s] = d2c[1][index];
                    c[2][s] = d2c[2][index];
                    c[3][s] = d2c[3][index];
                    index++;
                    if(index == 3 && e) break;
                }
            }
        }
        
    } else if (sg->type == MSYM_POINT_GROUP_TYPE_Dn && sg->n == 5){
        int sfound = 0, found = 0;
        msym_subgroup_t *ssg = NULL;
        for(int i = 0;i < 2; i++){
            if(sg->generators[i]->type == MSYM_POINT_GROUP_TYPE_Cn && sg->generators[i]->order == 2){
                ssg = sg->generators[i];
                break;
            }
        }
        if(NULL == ssg){
            ret = MSYM_INVALID_CHARACTER_TABLE;
            msymSetErrorDetails("Cannot find C2 subgroup of D5 %s");
            goto err;
        }
        cd[1] = cd[2] = 2;
        printf("setting c to 2\n");
        double d5c[2][5]= {
            [0] = {1, 1/PHI, -PHI},
            [1] = {1,  -PHI, 1/PHI}
        };
        
        double d2c[5] = {1,0,0};
        
        double c2c[5] = {1,-1};
        
        
        for(int s = 0;s < pg->order && (found < 10 || !sfound);s++){
            for(int i = 0;i < ssg->order && !sfound;i++){
                if(&pg->sops[s] != ssg->sops[i]) continue;
                if(pg->sops[s].type == PROPER_ROTATION && pg->sops[s].order == 2){
                    c[3][s] = c2c[0];
                    c[4][s] = c2c[1];
                    break;
                }
            }
            
            for(int i = 0;i < sg->order;i++){
                if(&pg->sops[s] != sg->sops[i]) continue;
                if(pg->sops[s].type == IDENTITY){
                    e = 1;
                    c[0][s] = c[3][s] = c[4][s] = 1;
                    c[1][s] = c[2][s] = 2;
                    found++;
                } else if (pg->sops[s].type == PROPER_ROTATION && pg->sops[s].order == 5) {
                    int index = (pg->sops[s].power >> 1) & 1; //2 and 3 -> index 1
                    c[0][s] = d5c[index][0];
                    c[1][s] = d5c[index][1];
                    c[2][s] = d5c[index][2];
                    found++;
                } else if(pg->sops[s].type == PROPER_ROTATION && pg->sops[s].order == 2){
                    c[0][s] = d2c[0];
                    c[1][s] = d2c[1];
                    c[2][s] = d2c[2];
                    found++;
                }
            }
        }
    }
    
    else {
        ret = MSYM_INVALID_CHARACTER_TABLE;
        msymSetErrorDetails("Cannot determine splitting field characters of subgroup %s",sg->name);
        goto err;
    }
    
err:
    return ret;
}

msym_error_t symmetrySpeciesComponents(msym_point_group_t *pg, int srsl, msym_subrepresentation_space_t *srs, int basisl, msym_basis_function_t *basis, double *wf, double *s){
    msym_error_t ret = MSYM_SUCCESS;
    
    if(srsl != pg->ct->d){
        ret = MSYM_SUBSPACE_ERROR;
        msymSetErrorDetails("Unexpected subspace length (expected %d got %d)",pg->ct->d, srsl);
        goto err;
    }
    
    for(int k = 0;k < srsl;k++){
        double kcomp = 0.0;
        for(int s = 0;s < srs[k].salcl;s++){
            msym_salc_t *salc = &srs[k].salc[s];
            double (*space)[salc->fl] = (double (*)[salc->fl]) salc->pf;
            for(int d = 0;d < salc->d;d++){
                double c = 0.0;
                for(int j = 0; j < salc->fl;j++){
                    c += wf[salc->f[j] - basis]*space[d][j];
                }
                kcomp += SQR(c);
            }
        }
        s[k] = sqrt(kcomp);
    }
    
err:
    
    return ret;
}


msym_error_t generateSubrepresentationSpaces(msym_point_group_t *pg, int sgl, const msym_subgroup_t sg[sgl], int esl, msym_equivalence_set_t *es, msym_permutation_t **perm, int basisl, msym_basis_function_t basis[basisl], msym_element_t *elements, msym_equivalence_set_t **esmap, msym_thresholds_t *thresholds, int *osrsl, msym_subrepresentation_space_t **osrs, msym_basis_function_t ***osrsbf, int **ospan){
    msym_error_t ret = MSYM_SUCCESS;
    msym_character_table_t *ct = pg->ct;
    int lmax = -1, nmax = 0;
    enum _msym_basis_type ftype = basis[0].type;
    for(int i = 0;i < basisl;i++){
        if(basis[i].type != ftype) {nmax = -1; break;}
        lmax = basis[i].f.rsh.l > lmax ? basis[i].f.rsh.l : lmax;
        nmax = basis[i].f.rsh.n > nmax ? basis[i].f.rsh.n : nmax;
    }
    
    if(lmax < 0 || nmax < 1){
        if(nmax == -1) msymSetErrorDetails("Basis functions are not of the same type");
        else msymSetErrorDetails("Invalid sperical harmonics quantum numbers");
        ret = MSYM_INVALID_BASIS_FUNCTIONS;
        return ret;
    } else if (ftype != MSYM_BASIS_TYPE_REAL_SPHERICAL_HARMONIC){
        msymSetErrorDetails("Basis function type not supported");
        ret = MSYM_INVALID_BASIS_FUNCTIONS;
        return ret;
    }
    
    int projm = (2*lmax+1)*pg->order;
    
    double (*bspan)[ct->d] = calloc(lmax+1, sizeof(*bspan));        // span of individual basis functions
    double (*pspan)[ct->d] = calloc(esl, sizeof(*pspan));               // span of permutation operators
    
    double (*lspan)[ct->d] = calloc(esl, sizeof(*lspan));           // total span of basis function on each ES
    double *rspan = calloc(ct->d, sizeof(*rspan));                  // total direct product symmetrized basis
    double *dspan = calloc(ct->d, sizeof(*dspan));                  // decoposed total span of symmetrized basis (double)
    
    double *mspan = calloc(ct->d, sizeof(double));                  // span decomposition memory
    double (*mproj)[projm] = calloc(projm, sizeof(*mproj));         // projection operator memory
    double (*mscal)[projm] = calloc(projm, sizeof(*mscal));         // projection scaling memory
    double (*mpih)[projm] = calloc(projm, sizeof(*mpih));           // icosahedral projection memory
    double (*mperm)[pg->order] = calloc(pg->order, sizeof(*mperm)); // permutation memory
    double (*morth)[pg->order] = calloc(pg->order, sizeof(*morth)); // permutation orthoginalization memory
    double (*mbasis)[projm] = calloc(basisl, sizeof(*mbasis));      // basis function coefficients
    double (*mdec)[projm] = calloc(basisl, sizeof(*mdec));          // directo product decomposition memory
    double (*sgc)[5][pg->order] = calloc(ct->d,sizeof(*sgc));
    
    const msym_subgroup_t **rsg = calloc(ct->d, sizeof(*rsg));
    
    double *mdcomp = NULL;
    double *mdproj = NULL;
    int *mdfound = NULL;
    
    int (*sgd)[5] = calloc(ct->d,sizeof(*sgd));
    int *ispan = calloc(ct->d, sizeof(int));                               // decoposed total span of symmetrized basis (int)
    int *isalc = calloc(ct->d, sizeof(int));                               // number of added salcs to irrep
    int *esnmax = calloc(esl, sizeof(int));                                     // max n in eqset
    
    msym_basis_function_t *(*esbfmap)[pg->order][nmax+1][lmax+1][2*lmax+1] = calloc(esl,sizeof(*esbfmap));
    
    
    msym_basis_function_t *(*srsbf) = calloc(basisl, sizeof(*srsbf));
    
    int (*srsbfmap)[nmax+1][lmax+1] = calloc(esl,sizeof(*srsbfmap));
    
    rsh_representations_t *lts = calloc(lmax+1,sizeof(*lts)); // transformation matrices for rsh basis functions
    
    int (*les)[lmax+1] = calloc(esl, sizeof(*les));                      // number of l-type basis functions in each ES
    
    msym_basis_function_t dbf = {.type = ftype};
    double (*ctable)[ct->d] = ct->table;
    
    msym_subrepresentation_space_t *srs = calloc(ct->d, sizeof(*srs));
    
    /* determine number of l-type basis functions in each ES */
    for(int o = 0;o < basisl;o++){
        les[esmap[basis[o].element - elements] - es][basis[o].f.rsh.l] += basis[o].f.rsh.m == 0;
    }
    
    if(MSYM_SUCCESS != (ret = generateRepresentations(pg->order+1, pg->order, pg->sops, lmax, lts))) goto err;
    
    
    for(int k = 0; k < ct->d;k++){
        if(ct->s[k].d > 1){
            if(MSYM_SUCCESS != (ret = findSplittingFieldSubgroup(pg, k, sgl, sg, thresholds, &rsg[k]))) goto err;
            if(MSYM_SUCCESS != (ret = getSplittingFieldCharacters(pg, rsg[k], sgc[k], sgd[k]))) goto err;
        }
    }
    
    
    for(int o = 0;o < basisl;o++){
        msym_basis_function_t *bf = &basis[o];
        int ei = (int)(bf->element - elements), esi = 0;
        msym_equivalence_set_t *e = esmap[ei];
        for(esi = 0;esi < e->length && e->elements[esi] != bf->element;esi++){}; //could improve perf with a map here
        if(esi >= e->length){
            ret = MSYM_INVALID_BASIS_FUNCTIONS;
            msymSetErrorDetails("Basis function does not map to any equivalence set");
            goto err;
        }
        if(bf->f.rsh.n > esnmax[e - es]) esnmax[e - es] = bf->f.rsh.n;
        esbfmap[e - es][esi][bf->f.rsh.n][bf->f.rsh.l][bf->f.rsh.m+bf->f.rsh.l] = bf;
    }
    
    int srsbfi = 0;
    
    for(int i = 0;i < esl;i++){
        for(int n = 0;n <= nmax;n++){
            for(int l = 0;l <= lmax;l++){
                srsbfmap[i][n][l] = srsbfi;
                for(int e = 0;e < es[i].length;e++){
                    msym_basis_function_t *lbf = esbfmap[i][e][n][l][0];
                    for(int m = -l;m <= l;m++){
                        msym_basis_function_t *bf = esbfmap[i][e][n][l][m+l];
                        if((NULL == bf && NULL != lbf) || (NULL == lbf && NULL != bf)) {
                            lbf = NULL == lbf ? bf : lbf;
                            ret = MSYM_INVALID_BASIS_FUNCTIONS;
                            msymSetErrorDetails("Found basis function %s but function where m = %d is missing on element %d of equivalence set %d",lbf->name,m,e,i);
                            goto err;
                        }
                        if(NULL != bf){
                            lbf = bf;
                            srsbf[srsbfi] = bf;
                            srsbfi++;
                        }
                    }
                }
                //when this is moved to a separate function it can be used and availability check (replace esbfmap)
                //if(srsbfmap[i][n][l] == srsbfi) srsbfmap[i][n][l] = -1;
            }
        }
    }
    
    if(srsbfi != basisl){
        ret = MSYM_SUBSPACE_ERROR;
        msymSetErrorDetails("Unexpected number of basis functions in subrepresentation map (expected %d, got %d)",basisl, srsbfi);
        goto err;
    }
    
    /* calculate span of irreducible representations for basis functions and permutations */
    for(int s = 0; s < pg->order;s++){
        for(int l = 0; l <= lmax;l++){
            dbf.f.rsh.l = l; //TODO: function type based
            double c = symmetryOperationCharacter(&pg->sops[s], &dbf);
            for(int k = 0;k < ct->d;k++){
                bspan[l][k] += c*ctable[k][pg->sops[s].cla];
            }
        }
        for(int i = 0;i < esl;i++){
            int uma = 0;
            for(int j = 0; j < perm[i][s].c_length;j++) uma += perm[i][s].c[j].l == 1; //this is why we loop over irreps twice
            for(int k = 0;k < ct->d;k++){
                pspan[i][k] += uma*ctable[k][pg->sops[s].cla];
            }
        }
    }
    
    /* scale reducible representations */
    for(int k = 0;k < ct->d;k++){
        double r = ct->s[k].r;
        if(r > 1){
            for(int l = 0; l <= lmax;l++) bspan[l][k] /= r;
            for(int i = 0;i < esl;i++) pspan[i][k] /= r;
        }
    }
    
    /* scale basis span, calculate basis function transforms and symmetry species vectors */
    for(int l = 0;l <= lmax;l++){
        int d = lts[l].d;
        double (*lproj)[d] = mproj;
        double (*lscal)[d] = mscal;
        
        vlscale(1.0/pg->order, ct->d, bspan[l], bspan[l]);
        
        double (*st)[d][d] = lts[l].t;
        memset(st[pg->order], 0, sizeof(double[d][d]));
        
        for(int k = 0, oirl = 0, nirl = 0;k < ct->d;k++, oirl = nirl){
            int vspan = ct->s[k].d*((int) round(bspan[l][k]));
            if(vspan == 0) continue;
            
            memset(lproj, 0, sizeof(double[d][d]));
            for(int s = 0;s < pg->order;s++){
                mlscale(ctable[k][pg->sops[s].cla], d, st[s], lscal);
                mladd(d, lscal, lproj, lproj);
            }
            
            mlscale(((double) ct->s[k].d)/pg->order, d, lproj, lproj);
            nirl = mgs(d, lproj, st[pg->order], oirl, thresholds->orthogonalization/basisl);
            
            if(nirl - oirl != vspan){
                debug_printTransform(d, d, st[pg->order]);
                ret = MSYM_SUBSPACE_ERROR;
                msymSetErrorDetails("Ortogonal subspace of dimension (%d) inconsistent with span (%d) in %s",nirl - oirl,vspan,ct->s[k].name);
                goto err;
                
            }
        }
        
        for(int i = 0; i < d;i++) vlnorm(d, st[pg->order][i]);
        
    }
    
    /* scale permutation span and calculate total basis function span on each ES */
    for(int i = 0;i < esl;i++){
        vlscale(1.0/pg->order, ct->d, pspan[i], pspan[i]);
        for(int l = 0; l <= lmax;l++){
            if(les[i][l] == 0) continue;
            les[i][l] /= es[i].length;
            vlscale((double) les[i][l], ct->d, bspan[l], mspan);
            vladd(ct->d, mspan, lspan[i], lspan[i]);
        }
    }
    
    /* calculate direct product of irreducible representations spanned by basis functions and permutations on each ES (don't really need to do this) */
    for(int i = 0;i < esl;i++){
        for(int k = 0;k < ct->d;k++){
            for(int j = 0;j < ct->d && round(pspan[i][k]) > 0;j++){
                if(round(lspan[i][j]) == 0) continue;
                directProduct(ct->d, ctable[k], ctable[j], mspan);
                vlscale(pspan[i][k]*lspan[i][j], ct->d, mspan, mspan);
                vladd(ct->d, mspan, rspan, rspan);
            }
        }
    }
    
    /* decompose direct product into irreducible representations */
    decomposeRepresentation(ct, rspan, dspan);
    int ddim_max = 0;
    for(int k = 0;k < ct->d;k++){
        ispan[k] = (int)round(dspan[k]/ct->s[k].r);
        ddim_max = ddim_max > ispan[k] ? ddim_max : ispan[k];
        srs[k].s = k;
        srs[k].salcl = ispan[k];
        srs[k].salc = calloc(srs[k].salcl, sizeof(msym_salc_t));
    }
    
    mdcomp = malloc(sizeof(double[5][ddim_max][ddim_max]));
    mdproj = malloc(sizeof(double[projm][projm]));
    mdfound = malloc(sizeof(int[5][ddim_max]));
    
    
    clean_debug_printf("decomposed %d\n", ct->d);
    for(int prk = 0;prk < ct->d;prk++){
        if(prk < ct->d - 1) clean_debug_printf("%d%s + ", ispan[prk], ct->s[prk].name);
        else clean_debug_printf("%d%s\n", ispan[prk], ct->s[prk].name);        
    }
    
    int dbasisl = 0;
    
    for(int k = 0;k < ct->d;k++){
        dbasisl += ct->s[k].d*ispan[k];
    }
    
    if(dbasisl != basisl){
        ret = MSYM_SUBSPACE_ERROR;
        msymSetErrorDetails("Unexpected number of basis functions in decomposition (expected %d, got %d)",basisl,dbasisl);
        goto err;
    }
    
    for(int i = 0; i < esl; i++){
        int d = es[i].length;
        double (*pproj)[d] = mproj, (*pscal)[d] = mscal, (*porth)[d] = morth;
        
        memset(porth, 0, sizeof(double[d][d]));
        for(int k = 0, oirl = 0, nirl = 0;k < ct->d;k++, oirl = nirl){
            int vspan = ct->s[k].d*((int) round(pspan[i][k]));
            if(vspan == 0) continue;
            
            memset(pproj, 0, sizeof(double[d][d]));
            for(int s = 0;s < pg->order;s++){
                if(ctable[k][pg->sops[s].cla] == 0) continue;
                permutationMatrix(&perm[i][s], mperm);
                mlscale(ctable[k][pg->sops[s].cla], d, mperm, pscal);
                mladd(d, pscal, pproj, pproj);
            }
            
            mlscale(((double) ct->s[k].d)/pg->order, d, pproj, pproj);
            nirl = mgs(d, pproj, porth, oirl, thresholds->orthogonalization/basisl);
            
            if(nirl - oirl != vspan){
                ret = MSYM_SUBSPACE_ERROR;
                msymSetErrorDetails("Ortogonal ES subspace of dimension (%d) inconsistent with span (%d) in %s",nirl - oirl,vspan,ct->s[k].name);
                goto err;
                
            }
            
            for(int oi = oirl; oi < nirl;oi++) vlnorm(d, porth[oi]);
            for(int l = 0;l <= lmax;l++){
                if(les[i][l] <= 0) continue;
                int li = 0, ld = lts[l].d;
                double (*lst)[ld][ld] = lts[l].t;
                for(int lk = 0;lk < ct->d;lk++){
                    int lvspan = ct->s[lk].d*((int) round(bspan[l][lk])), dd = d*ld;
                    if(lvspan == 0) continue;
                    kron2(vspan, d, &porth[oirl], lvspan, ld, &lst[pg->order][li], mbasis);
                    li += lvspan;
                    
                    directProduct(ct->d, ctable[k], ctable[lk], rspan);
                    vlscale(pspan[i][k]*bspan[l][lk], ct->d, rspan, rspan);
                    decomposeRepresentation(ct, rspan, mspan);
                    
                    for(int dk = 0; dk < ct->d; dk++) mspan[dk] /= ct->s[dk].r;
                    
                    if(ct->s[k].d > 1 && ct->s[lk].d > 1){
                        memcpy(mdec, mbasis, sizeof(double[vspan*lvspan][dd]));
                        memset(mbasis, 0, sizeof(double[vspan*lvspan][dd]));
                        for(int dk = 0, doirl = 0, dnirl = 0;dk < ct->d;dk++, doirl = dnirl){
                            int dvspan = ct->s[dk].d*((int) round(mspan[dk]));
                            if(dvspan == 0) continue;
                            
                            double (*dproj)[dd] = mproj; // projection operator for direct product
                            double (*dscal)[dd] = mscal;
                            
                            memset(dproj, 0, sizeof(double[dd][dd]));
                            
                            for(int s = 0;s < pg->order;s++){
                                if(ctable[dk][pg->sops[s].cla] == 0) continue;
                                permutationMatrix(&perm[i][s], mperm);
                                kron(d, mperm, ld, lst[s], dd, dscal);
                                
                                mlscale(ctable[dk][pg->sops[s].cla], dd, dscal, dscal);
                                mladd(dd, dscal, dproj, dproj);
                            }
                            
                            mlscale(((double) ct->s[dk].d)/pg->order, dd, dproj, dproj);
                            mmtlmul(dd,dd,dproj,vspan*lvspan,mdec,dscal);
                            memset(dproj, 0, sizeof(double[dd][dd]));
                            mltranspose(dd, vspan*lvspan, dscal, dproj);
                            
                            dnirl = mgs(dd, dproj, mbasis, doirl, thresholds->orthogonalization/basisl);
                            if(dnirl - doirl != dvspan){
                                ret = MSYM_SUBSPACE_ERROR;
                                msymSetErrorDetails("Ortogonal subspace decomposition of dimension (%d) inconsistent with span (%d) in %s",dnirl - doirl,vspan,ct->s[dk].name);
                                goto err;
                                
                            }
                        }
                    }
                    for(int sk = 0,si=0;sk < ct->d;sk++){
                        int svspan = ct->s[sk].d*((int) round(mspan[sk])), dd = d*ld;
                        if(svspan == 0) continue;
                        double (*sbasis)[dd] = mbasis;
                        double (*sdec)[dd] = mdec;
                        if(ct->s[sk].d > 1){
                            //icosahedral symmetry map (dependant on getSplittingFieldCharacters)
                            int ihdim[] = {0,1,1,2,2};
                            int ihsub[] = {0,3,4,3,4};
                            double (*ihproj)[dd] = mpih;
                            double (*dproj)[dd] = mproj; // projection operator for direct product
                            double (*dscal)[dd] = mscal;
                            
                            memset(dproj, 0, sizeof(double[dd][dd]));
                            memset(mdec, 0, sizeof(double[vspan*lvspan][dd]));
                            for(int dim = 0, doirl = 0, dnirl = 0;dim < ct->s[sk].d;dim++, doirl = dnirl){
                                int pdim = ct->s[sk].d == 5 ? ihdim[dim] : dim; //icosahedral symmetry has split character table
                                memset(dproj, 0, sizeof(double[dd][dd]));
                                for(int s = 0;s < pg->order;s++){
                                    if(sgc[sk][pdim][s] == 0) continue;
                                    // we really should have precomputed this by now, it's the third time,
                                    // or a better approach would be a an optimized function
                                    permutationMatrix(&perm[i][s], mperm);
                                    kron(d, mperm, ld, lst[s], dd, dscal);
                                    mlscale(sgc[sk][pdim][s], dd, dscal, dscal);
                                    mladd(dd, dscal, dproj, dproj);
                                }
                                if(sgd[sk][pdim] == 2){ // only icosahedral
                                    int c2dim = ihsub[dim];
                                    memset(ihproj, 0, sizeof(double[dd][dd]));
                                    //getting rediculous
                                    for(int s = 0;s < pg->order;s++){
                                        if(sgc[sk][c2dim][s] == 0) continue;
                                        permutationMatrix(&perm[i][s], mperm);
                                        kron(d, mperm, ld, lst[s], dd, dscal);
                                        mlscale(sgc[sk][c2dim][s], dd, dscal, dscal);
                                        mladd(dd, dscal, ihproj, ihproj);
                                    }
                                    mmlmul(dd, dd, ihproj, dd, dproj, dscal); //avoid the mul malloc
                                    memcpy(dproj, dscal, sizeof(double[dd][dd]));
                                }
                                
                                
                                
                                mlscale(1.0/rsg[sk]->order, dd, dproj, dproj);
                                
                                //printf("constructed subgroup projection operator %s\n",cs_name[dim]);
                                //debug_printTransform(dd, dd, dproj);
                                
                                mmtlmul(dd,dd,dproj,svspan,&sbasis[si],dscal);
                                
                                memset(dproj, 0, sizeof(double[dd][dd]));
                                mltranspose(dd, svspan, dscal, dproj);
                                dnirl = mgs(dd, dproj, sdec, doirl, thresholds->orthogonalization/basisl);
                                if(dnirl - doirl != round(mspan[sk])){
                                    ret = MSYM_SUBSPACE_ERROR;
                                    msymSetErrorDetails("Multi-dimensional subspace decomposition of %s dimension (%d) inconsistent with representaion (%d) in subgroup irrep %d",ct->s[sk].name, dnirl - doirl,(int) round(mspan[sk]),dim);
                                    goto err;
                                }
                                
                                for(int oi = doirl; oi < dnirl;oi++) vlnorm(dd, sdec[oi]);
                                
                            }
                            //do apply symmetry here
                            memcpy(&sbasis[si], sdec, sizeof(double[svspan][dd]));
                            
                            int mdim = round(mspan[sk]);
                            int (*found)[mdim] = (int (*)[mdim]) mdfound;
                            double (*g)[mdim][mdim] = (double (*)[mdim][mdim]) mdcomp;
                            double (*mt)[mdim] = (double (*)[mdim]) mdproj;
                            
                            memset(found,0,sizeof(int[ct->s[sk].d][mdim]));
                            for(int s = 0;s < pg->order ;s++){
                                permutationMatrix(&perm[i][s], mperm);
                                kron(d, mperm, ld, lst[s], dd, dscal);
                                for(int dim = 0;dim < mdim;dim++){
                                    for(int sg = 1;sg < ct->s[sk].d;sg++){
                                        if(found[sg][dim]) continue;
                                        mmtlmul(dd, dd, dscal, mdim, &sdec[sg*mdim], mt);
                                        mmlmul(1,dd,&sdec[dim], mdim, mt, &g[sg][dim]);
                                        if(vlabs(mdim, g[sg][dim]) > thresholds->zero) found[sg][dim] = 1;
                                    }
                                }
                            }
                            
                            for(int cdim = 0;cdim < mdim;cdim++){
                                vlnorm2(dd, sdec[cdim], sbasis[si+ct->s[sk].d*cdim]);
                            }
                            for(int dim = 1; dim < ct->s[sk].d;dim++){
                                mmlmul(mdim, mdim, g[dim], dd, &sdec[dim*mdim], dproj);
                                
                                for(int cdim = 0;cdim < mdim;cdim++){
                                    vlnorm2(dd, dproj[cdim], sbasis[si+dim+ct->s[sk].d*cdim]);
                                }
                            }
                        }
                        for(int ir = 0;ir < svspan;ir += ct->s[sk].d){
                            for(int n = l+1; n <= esnmax[i];n++){
                                if(esbfmap[i][0][n][l][0] == NULL) continue;
                                if(isalc[sk] >= ispan[sk]){
                                    ret = MSYM_SUBSPACE_ERROR;
                                    msymSetErrorDetails("Exceeded calculated number of SALCs %d >= %d",isalc[sk],ispan[sk]);
                                    goto err;
                                }
                                //printf("adding salc %d of %d %d\n",isalc[sk],ispan[sk],sk);
                                msym_salc_t *salc = &srs[sk].salc[isalc[sk]];
                                salc->d = ct->s[sk].d;
                                
                                double (*pf)[dd] = calloc(salc->d,sizeof(double[dd]));
                                
                                for(int dim = 0; dim < salc->d;dim++){
                                    vlnorm2(dd, sbasis[si+dim+ir], pf[dim]);
                                }
                                salc->pf = (double*) pf;
                                salc->fl = dd;
                                salc->f = &srsbf[srsbfmap[i][n][l]];
                                
                                isalc[sk]++;
                            }
                        }
                        si += svspan;
                    }
                }
            }
        }
    }
    
    debug_printSubspace(ct,ct->d,srs);
    *ospan = ispan;
    *osrsl = ct->d;
    *osrs = srs;
    *osrsbf = srsbf;
    
    free(bspan);
    free(pspan);
    free(lspan);
    free(rspan);
    free(dspan);
    free(mspan);
    free(mproj);
    free(mscal);
    free(mpih);
    free(mperm);
    free(morth);
    free(mbasis);
    free(mdec);
    free(rsg);
    free(sgc);
    free(sgd);
    free(mdcomp);
    free(mdproj);
    free(mdfound);
    free(isalc);
    free(esnmax);
    free(esbfmap);
    for(int l = 0;l <= lmax;l++){
        free(lts[l].t);
    }
    free(lts);
    free(les);
    free(srsbfmap);
    
    return ret;
    
err:
    free(bspan);
    free(pspan);
    free(lspan);
    free(rspan);
    free(dspan);
    free(mspan);
    free(mproj);
    free(mscal);
    free(mperm);
    free(morth);
    free(mbasis);
    free(mdec);
    free(rsg);
    free(sgc);
    free(mdcomp);
    free(mdproj);
    free(mdfound);
    free(ispan);
    free(isalc);
    free(esnmax);
    free(esbfmap);
    for(int l = 0;l <= lmax;l++){
        free(lts[l].t);
    }
    free(lts);
    free(les);
    free(srsbfmap);
    for(int k = 0;k < ct->d;k++){
        for(int i = 0;i < srs[k].salcl;i++){
            free(srs[k].salc[i].pf);
        }
        free(srs[k].salc);
    }
    free(srs);
    free(srsbf);
    
    return ret;
}

void freeSubrepresentationSpaces(int srsl, msym_subrepresentation_space_t *srs){
    for(int i = 0;i < srsl && NULL != srs;i++){
        for(int j = 0;j < srs[i].salcl;j++){
            free(srs[i].salc[j].pf);
        }
        free(srs[i].salc);
    }
    free(srs);
}

