import React, { SyntheticEvent,Component } from 'react';
import IconButton from '@material-ui/core/IconButton';
import Typography from '@material-ui/core/Typography';
import { PiPedalModel, PiPedalModelFactory, PresetIndexEntry, PresetIndex } from './PiPedalModel';
import Button from "@material-ui/core/Button";
import ButtonBase from "@material-ui/core/ButtonBase";
import { TransitionProps } from '@material-ui/core/transitions/transition';
import Slide from '@material-ui/core/Slide';
import Dialog from '@material-ui/core/Dialog';
import AppBar from '@material-ui/core/AppBar';
import Toolbar from '@material-ui/core/Toolbar';
import { Theme, withStyles, WithStyles, createStyles } from '@material-ui/core/styles';
import DraggableGrid, { ScrollDirection } from './DraggableGrid';
import MoreVertIcon from '@material-ui/icons/MoreVert';
import ListItemIcon from '@material-ui/core/ListItemIcon';
import ListItemText from '@material-ui/core/ListItemText';
import Menu from '@material-ui/core/Menu';
import MenuItem from '@material-ui/core/MenuItem';
import Fade from '@material-ui/core/Fade';
import UploadDialog from './UploadDialog';

import SelectHoverBackground from './SelectHoverBackground';
import CloseIcon from '@material-ui/icons/Close';
import ArrowBackIcon from '@material-ui/icons/ArrowBack';
import EditIcon from '@material-ui/icons/Edit';
import RenameDialog from './RenameDialog';

interface PresetDialogProps extends WithStyles<typeof styles> {
    show: boolean;
    isEditDialog: boolean;
    onDialogClose: () => void;

};

interface PresetDialogState {
    presets: PresetIndex;

    showActionBar: boolean;

    selectedItem: number;

    renameOpen: boolean;

    moreMenuAnchorEl: HTMLElement | null;
    openUploadDialog: boolean;


};


const styles = (theme: Theme) => createStyles({
    dialogAppBar: {
        position: 'relative',
        top: 0, left: 0
    },
    dialogActionBar: {
        position: 'relative',
        top: 0, left: 0,
        background: "black"
    },
    dialogTitle: {
        marginLeft: theme.spacing(2),
        flex: 1,
    },
    itemFrame: {
        display: "flex",
        flexDirection: "row",
        flexWrap: "nowrap",
        width: "100%",
        height: "56px",
        alignItems: "center",
        textAlign: "left",
        justifyContent: "center",
        paddingLeft: 8
    },
    iconFrame: {
        flex: "0 0 auto",

    },
    itemIcon: {
        width: 24, height: 24, margin: 12, opacity: 0.6
    },
    itemLabel: {
        flex: "1 1 1px",
        marginLeft: 8
    }

});


const Transition = React.forwardRef(function Transition(
    props: TransitionProps & { children?: React.ReactElement },
    ref: React.Ref<unknown>,
) {
    return <Slide direction="up" ref={ref} {...props} />;
});

// eslint-disable-next-line @typescript-eslint/no-unused-vars
const ActionBarTransition = React.forwardRef(function Transition(
    props: TransitionProps & { children?: React.ReactElement },
    ref: React.Ref<unknown>,
) {
    return <Slide direction="right" ref={ref} {...props} />;
});


const PresetDialog = withStyles(styles, { withTheme: true })(

    class extends Component<PresetDialogProps, PresetDialogState> {

        model: PiPedalModel;



        constructor(props: PresetDialogProps) {
            super(props);
            this.model = PiPedalModelFactory.getInstance();

            this.handleDialogClose = this.handleDialogClose.bind(this);
            let presets = this.model.presets.get();
            this.state = {
                presets: presets,
                showActionBar: false,
                selectedItem: presets.selectedInstanceId,
                renameOpen: false,
                moreMenuAnchorEl: null,
                openUploadDialog: false

            };
            this.handlePresetsChanged = this.handlePresetsChanged.bind(this);

        }

        selectItemAtIndex(index: number) {
            let instanceId = this.state.presets.presets[index].instanceId;
            this.setState({ selectedItem: instanceId });
        }
        isEditMode() {
            return this.state.showActionBar || this.props.isEditDialog;
        }

        onMoreClick(e: SyntheticEvent): void {
            this.setState({ moreMenuAnchorEl: e.currentTarget as HTMLElement })
        }

        handleDownloadPreset() {
            this.handleMoreClose();
            this.model.download("downloadPreset", this.model.presets.get().selectedInstanceId);
        }
        handleUploadPreset() {
            this.handleMoreClose();
            this.setState({ openUploadDialog: true });

        }
        handleMoreClose(): void {
            this.setState({ moreMenuAnchorEl: null });

        }


        handlePresetsChanged() {
            let presets = this.model.presets.get();

            if (!presets.areEqual(this.state.presets, false)) // avoid a bunch of peculiar effects if we update while a drag is in progress
            {
                // if we don't have a valid selection, then use the current preset.
                if (this.state.presets.getItem(this.state.selectedItem) == null) {
                    this.setState({ presets: presets, selectedItem: presets.selectedInstanceId });
                } else {
                    this.setState({ presets: presets });
                }
            }
        }


        componentDidMount() {
            this.model.presets.addOnChangedHandler(this.handlePresetsChanged);
            this.handlePresetsChanged();
            // scroll selected item into view.
        }
        componentWillUnmount() {
            this.model.presets.removeOnChangedHandler(this.handlePresetsChanged);
        }

        getSelectedIndex() {
            let instanceId = this.isEditMode() ? this.state.selectedItem : this.state.presets.selectedInstanceId;
            let presets = this.state.presets;
            for (let i = 0; i < presets.presets.length; ++i) {
                if (presets.presets[i].instanceId === instanceId) return i;
            }
            return -1;
        }

        handleDeleteClick() {
            if (!this.state.selectedItem) return;
            let selectedItem = this.state.selectedItem;
            if (selectedItem !== -1) {
                this.model.deletePresetItem(selectedItem)
                    .then((selectedItem: number) => {
                        this.setState({ selectedItem: selectedItem });
                    })
                    .catch((error) => {
                        this.model.showAlert(error);
                    });

            }
        }
        handleDialogClose() {
            this.props.onDialogClose();
        }

        handleItemClick(instanceId: number): void {
            if (this.isEditMode()) {
                this.setState({ selectedItem: instanceId });
            } else {
                this.model.loadPreset(instanceId);
                this.props.onDialogClose();
            }
        }
        showActionBar(show: boolean): void {
            this.setState({ showActionBar: show });

        }


        mapElement(el: any): React.ReactNode {
            let presetEntry = el as PresetIndexEntry;
            let classes = this.props.classes;
            let selectedItem = this.isEditMode() ? this.state.selectedItem : this.state.presets.selectedInstanceId;
            return (
                <div key={presetEntry.instanceId} style={{ background: "white" }} >

                    <ButtonBase style={{ width: "100%", height: 48 }}
                        onClick={() => this.handleItemClick(presetEntry.instanceId)}
                    >
                        <SelectHoverBackground selected={presetEntry.instanceId === selectedItem} showHover={true} />
                        <div className={classes.itemFrame}>
                            <div className={classes.iconFrame}>
                                <img src="img/ic_presets.svg" className={classes.itemIcon} alt="" />
                            </div>
                            <div className={classes.itemLabel}>
                                <Typography>
                                    {presetEntry.name}
                                </Typography>
                            </div>
                        </div>
                    </ButtonBase>
                </div>

            );
        }

        updateServerPresets(newPresets: PresetIndex) {
            newPresets = newPresets.clone();
            this.model.updatePresets(newPresets)
                .catch((error) => {
                    this.model.showAlert(error);
                });
        }
        moveElement(from: number, to: number): void {
            let newPresets = this.state.presets.clone();
            newPresets.movePreset(from, to);
            this.setState({
                presets: newPresets,
                selectedItem: newPresets.presets[to].instanceId
            });
            this.updateServerPresets(newPresets);
        }

        getSelectedName(): string {
            let item = this.state.presets.getItem(this.state.selectedItem);
            if (item) return item.name;
            return "";
        }

        handleRenameClick() {
            let item = this.state.presets.getItem(this.state.selectedItem);
            if (item) {
                this.setState({ renameOpen: true });
            }
        }
        handleRenameOk(text: string) {
            let item = this.state.presets.getItem(this.state.selectedItem);
            if (!item) return;
            if (item.name !== text) {
                this.model.renamePresetItem(this.state.selectedItem, text)
                    .catch((error) => {
                        this.onError(error);
                    });
            }

            this.setState({ renameOpen: false });
        }
        handleCopy() {
            let item = this.state.presets.getItem(this.state.selectedItem);
            if (!item) return;
            this.model.duplicatePreset(this.state.selectedItem)
                .then((newId) => {
                    this.setState({ selectedItem: newId });
                }).catch((error) => {
                    this.onError(error);
                });
        }

        onError(error: string): void {
            this.model?.showAlert(error);
        }



        render() {
            let classes = this.props.classes;

            let actionBarClass = this.props.isEditDialog ? classes.dialogAppBar : classes.dialogActionBar;
            let defaultSelectedIndex = this.getSelectedIndex();

            return (
                <Dialog fullScreen open={this.props.show}
                    onClose={() => { this.handleDialogClose() }} TransitionComponent={Transition}>
                    <div style={{ display: "flex", flexDirection: "column", flexWrap: "nowrap", width: "100%", height: "100%", overflow: "hidden" }}>
                        <div style={{ flex: "0 0 auto" }}>
                            <AppBar className={classes.dialogAppBar} style={{ display: this.isEditMode() ? "none" : "block" }} >
                                <Toolbar>
                                    <IconButton edge="start" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                        disabled={this.isEditMode()}
                                    >
                                        <ArrowBackIcon />
                                    </IconButton>
                                    <Typography variant="h6" className={classes.dialogTitle}>
                                        Presets
                                    </Typography>
                                    <IconButton color="inherit" onClick={(e) => this.showActionBar(true)} >
                                        <EditIcon />
                                    </IconButton>
                                </Toolbar>
                            </AppBar>
                            <AppBar className={actionBarClass} style={{ display: this.isEditMode() ? "block" : "none" }}
                                onClick={(e) => { e.stopPropagation(); e.preventDefault(); }}
                            >
                                <Toolbar>
                                    {(!this.props.isEditDialog) ? (
                                        <IconButton edge="start" color="inherit" onClick={(e) => this.showActionBar(false)} aria-label="close">
                                            <CloseIcon />
                                        </IconButton>
                                    ) : (
                                        <IconButton edge="start" color="inherit" onClick={this.handleDialogClose} aria-label="back"
                                        >
                                            <ArrowBackIcon />
                                        </IconButton>

                                    )}
                                    <Typography variant="h6" className={classes.dialogTitle}>
                                        Presets
                                    </Typography>
                                    {(this.state.presets.getItem(this.state.selectedItem) != null)
                                        && (

                                            <div style={{ flex: "0 0 auto" }}>
                                                <Button color="inherit" onClick={(e) => this.handleCopy()}>
                                                    Copy
                                                </Button>
                                                <Button color="inherit" onClick={() => this.handleRenameClick()}>
                                                    Rename
                                                </Button>
                                                <RenameDialog
                                                    open={this.state.renameOpen}
                                                    defaultName={this.getSelectedName()}
                                                    acceptActionName={"Rename"}
                                                    onClose={() => { this.setState({ renameOpen: false }) }}
                                                    onOk={(text: string) => {
                                                        this.handleRenameOk(text);
                                                    }
                                                    }
                                                />
                                                <IconButton color="inherit" onClick={(e) => this.handleDeleteClick()} >
                                                    <img src="/img/old_delete_outline_white_24dp.svg" alt="Delete" style={{ width: 24, height: 24, opacity: 0.6 }} />
                                                </IconButton>
                                                <IconButton color="inherit" onClick={(e) => { this.onMoreClick(e) }} >
                                                    <MoreVertIcon />
                                                </IconButton>
                                                <Menu
                                                    id="more-menu"
                                                    anchorEl={this.state.moreMenuAnchorEl}
                                                    keepMounted
                                                    open={Boolean(this.state.moreMenuAnchorEl)}
                                                    onClose={() => this.handleMoreClose()}
                                                    TransitionComponent={Fade}
                                                >
                                                    <MenuItem onClick={() => { this.handleDownloadPreset(); }} >
                                                        <ListItemIcon>
                                                            <img src="img/file_download_black_24dp.svg" style={{ width: 24, height: 24, opacity: 0.6 }} alt="" />
                                                        </ListItemIcon>
                                                        <ListItemText>
                                                            Download preset
                                                        </ListItemText>

                                                    </MenuItem>
                                                    <MenuItem onClick={() => { this.handleUploadPreset() }}>
                                                        <ListItemIcon>
                                                            <img src="img/file_upload_black_24dp.svg" style={{ width: 24, height: 24, opacity: 0.6 }} alt="" />
                                                        </ListItemIcon>
                                                        <ListItemText>
                                                            Upload preset
                                                        </ListItemText>

                                                    </MenuItem>
                                                </Menu>


                                            </div>
                                        )
                                    }
                                </Toolbar>

                            </AppBar>
                        </div>
                        <div style={{ flex: "1 1 auto", position: "relative", overflow: "hidden" }} >
                            <DraggableGrid
                                onLongPress={(item) => this.showActionBar(true)}
                                canDrag={this.isEditMode()}
                                onDragStart={(index, x, y) => { this.selectItemAtIndex(index) }}
                                moveElement={(from, to) => { this.moveElement(from, to); }}
                                scroll={ScrollDirection.Y}
                                defaultSelectedIndex={defaultSelectedIndex}
                            >
                                {
                                    this.state.presets.presets.map((element) => {
                                        return this.mapElement(element);
                                    })
                                }
                            </DraggableGrid>
                        </div>
                    </div>
                    <UploadDialog 
                            onUploaded={(instanceId) => this.setState({selectedItem: instanceId}) }
                            uploadAfter={this.state.selectedItem} 
                            open={this.state.openUploadDialog} 
                            onClose={() => { this.setState({ openUploadDialog: false }) }} />

                </Dialog >

            );

        }

    }

);

export default PresetDialog;